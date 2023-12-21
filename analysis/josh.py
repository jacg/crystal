from sys import argv

from pathlib import Path
import itertools as it

import numpy as np
import matplotlib.pyplot as plt
import polars as pl

import torch
import torch.nn as nn
import torch.optim as optim
import torch.nn.functional as F
from torch.utils.data import Dataset
from torch.utils.data import DataLoader

pixel_size = 6  # 6 mm x 6 mm pixels
grid_size = 8   # 8x8 events

_, datadir = argv

# Basic CNN for (x,y,z) prediction

chi = 128        # initial number of channels (increase to make net larger)
netdebug = False  # print debugging information for net

class CNN_basic(nn.Module):
    def __init__(self):
        super(CNN_basic, self).__init__()

        self.conv1 = nn.Conv2d(1, chi, 3, padding=1)
        self.bn1   = nn.BatchNorm2d(chi)
        self.conv2 = nn.Conv2d(chi, chi*2, 2, padding=1)
        self.bn2   = nn.BatchNorm2d(chi*2)
        self.conv3 = nn.Conv2d(chi*2, chi*4, 2, padding=1)
        self.bn3   = nn.BatchNorm2d(chi*4)
        self.pool = nn.MaxPool2d(2, 2)
        self.fc0 = nn.Linear(chi*4, 3)
        self.drop1 = nn.Dropout(p=0.2)

        # Initialize weights
#         for m in self.modules():
#             if isinstance(m, nn.Conv2d):
#                 nn.init.kaiming_normal_(m.weight, mode='fan_out', nonlinearity='relu')
#             elif isinstance(m, (nn.BatchNorm2d, nn.GroupNorm)):
#                 nn.init.constant_(m.weight, 1)
#                 nn.init.constant_(m.bias, 0)

    def forward(self, x):

        if(netdebug): print(x.shape)
        x = self.pool(self.bn1(F.leaky_relu(self.conv1(x))))
        if(netdebug): print(x.shape)
        x = self.pool(self.bn2(F.leaky_relu(self.conv2(x))))
        if(netdebug): print(x.shape)
        x = self.pool(self.bn3(F.leaky_relu(self.conv3(x))))
        if(netdebug): print(x.shape)
        x = x.flatten(start_dim=1)
        if(netdebug): print(x.shape)
        #x = self.drop1(x)
        x = self.fc0(x)
        if(netdebug): print(x.shape)

        return x


def polars_primary_positions(df):
    return (df.select("x y z".split())).to_numpy()

def polars_images(df):
    return (df.select(pl.col("photon_counts").flatten())
              .to_numpy()
              .flatten()
              .reshape(len(df), 8, 8)
              .astype(np.float32)
            )


class SiPMDataset(Dataset):

    def __init__(self, data_path, max_files=None):
        self.data_path = Path(data_path)

        positions = []
        images    = []
        for file in it.islice(self.data_path.glob('file-*.parquet'), max_files):
            print(file)
            table = pl.read_parquet(file, columns='x y z photon_counts'.split())
            single_file_positions = polars_primary_positions(table)
            single_file_images    = polars_images           (table)
            positions.append(single_file_positions)
            images   .append(single_file_images)
        self.positions = np.concatenate(positions)
        self.images    = np.concatenate(images)

    def __len__(self):
        return self.positions.shape[0]

    def __getitem__(self, idx):
        image    = self.images   [idx]
        position = self.positions[idx]
        image    = torch.tensor(image   , dtype=torch.float).unsqueeze(0) # Add channel dimension
        position = torch.tensor(position, dtype=torch.float)

        return image, position

    def plot_event(self, event_number):
        # Show the image.
        plt.imshow(self.images[event_number])

        x,y,z = self.positions[event_number]
        # Show the event location.
        x_evt = (y + pixel_size*grid_size/2)/pixel_size - 0.5
        y_evt = (x + pixel_size*grid_size/2)/pixel_size - 0.5
        plt.plot([x_evt],[y_evt],'o',color='red')
        plt.title(f'z = {z:.1f}')
        plt.colorbar()
        plt.show()



# Load the data
max_files = None  # Limit number of files to use
batch_size = 1000  # Batch size

dataset = SiPMDataset(datadir, max_files)

# Plot some events
first_event = 42
n_to_show = 1
for evt_no in range(first_event, first_event + n_to_show):
    dataset.plot_event(evt_no)

data_loader = DataLoader(dataset, batch_size=batch_size, shuffle=True)
ntot_evts = len(dataset)
print(f"Loaded {len(dataset)} events")

# Split the data into training, validation, and test sets
train_size = int(0.7 * len(dataset))  # training
val_size   = int(0.2 * len(dataset))    # validation
test_size = len(dataset) - train_size - val_size  # test
train_indices = range(train_size)
val_indices   = range(train_size, train_size + val_size)
test_indices  = range(train_size + val_size, len(dataset))

# Define subsets of the dataset
train_dataset = torch.utils.data.Subset(dataset, train_indices)
print(f"{len(train_dataset)} training events ({100*len(train_dataset)/ntot_evts}%)")
val_dataset = torch.utils.data.Subset(dataset, val_indices)
print(f"{len(val_dataset)} validation events ({100*len(val_dataset)/ntot_evts}%)")
test_dataset = torch.utils.data.Subset(dataset, test_indices)
print(f"{len(test_dataset)} test events ({100*len(test_dataset)/ntot_evts}%)")

# Define DataLoaders
train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
val_loader   = DataLoader(val_dataset  , batch_size=batch_size, shuffle=False)
test_loader  = DataLoader(test_dataset , batch_size=batch_size, shuffle=False)

# Load the model.
model = CNN_basic()
if torch.cuda.is_available():
    model = model.cuda()

# Set up the optimizer and loss function.
optimizer = optim.Adam(model.parameters(), lr=0.001)
criterion = nn.MSELoss()

# Training loop
epochs = 3
train_losses, val_losses = [], []
for epoch in range(epochs):

    train_losses_epoch, val_losses_epoch = [], []

    print(f"\nEPOCH {epoch}")

    # Training step
    for i, (images, positions) in enumerate(train_loader):

        if torch.cuda.is_available():
           images = images.cuda()
           positions = positions.cuda()

        model.train()
        optimizer.zero_grad()
        outputs = model(images)
        loss = criterion(outputs, positions)
        loss.backward()
        optimizer.step()
        train_losses_epoch.append(loss.data.item())
        if((i+1) % (len(train_loader)/10) == 0):
            print(f"Train Step {i + 1}/{len(train_loader)}, Loss: {loss.data.item()}")

    # Validation step
    with torch.no_grad():
        model.eval()
        for i, (images, positions) in enumerate(val_loader):

            if torch.cuda.is_available():
              images = images.cuda()
              positions = positions.cuda()

            outputs = model(images)
            loss = criterion(outputs, positions)
            val_losses_epoch.append(loss.data.item())
            if((i+1) % (len(val_loader)/10) == 0):
              print(f"Validation Step {i + 1}/{len(val_loader)}, Loss: {loss.data.item()}")


    train_losses.append(np.mean(train_losses_epoch))
    val_losses.append(np.mean(val_losses_epoch))
    print(f"--- EPOCH {epoch} AVG TRAIN LOSS: {np.mean(train_losses_epoch)}")
    print(f"--- EPOCH {epoch} AVG VAL LOSS: {np.mean(val_losses_epoch)}")

# Plot training and validation loss
xvals_train = np.arange(0,epochs,1)
xvals_val = np.arange(0,epochs,1)
plt.plot(xvals_train,train_losses,label='training')
plt.plot(xvals_val,val_losses,label='validation')
plt.xlabel("Epoch",fontsize=14)
plt.ylabel("Loss")
plt.legend()

def weighted_mean_and_sigma(image):

    # Total intensity of the image
    total_intensity = np.sum(image)

    # Indices for x and y (make (0,0) the center of the 8x8 grid)
    y_indices, x_indices = np.meshgrid(np.arange(image.shape[1]), np.arange(image.shape[0]))
    y_indices = np.array(y_indices) - 3.5
    x_indices = np.array(x_indices) - 3.5

    # Weighted means
    weighted_mean_x = np.sum(x_indices * image) / total_intensity
    weighted_mean_y = np.sum(y_indices * image) / total_intensity

    # Weighted standard deviations
    weighted_sigma_x = np.sqrt(np.sum(image * (x_indices - weighted_mean_x)**2) / total_intensity)
    weighted_sigma_y = np.sqrt(np.sum(image * (y_indices - weighted_mean_y)**2) / total_intensity)

    return weighted_mean_x, weighted_mean_y, weighted_sigma_x, weighted_sigma_y

# Evaluate the test set.
true_x, true_y, true_z = [],[],[]
mean_x, mean_y = [],[]
sigma_x, sigma_y = [],[]
predicted_x, predicted_y, predicted_z = [],[],[]
with torch.no_grad():

    model.eval()
    for i, (images, positions) in enumerate(test_loader):

        if torch.cuda.is_available():
            images = images.cuda()

        outputs = model(images).cpu()

        for x in positions[:,0]: true_x.append(x)
        for y in positions[:,1]: true_y.append(y)
        for z in positions[:,2]: true_z.append(z)

        for x in outputs[:,0]: predicted_x.append(x)
        for y in outputs[:,1]: predicted_y.append(y)
        for z in outputs[:,2]: predicted_z.append(z)

        for img in images.cpu().squeeze().numpy():
            mu_x, mu_y, sd_x, sd_y = weighted_mean_and_sigma(img)
            mean_x.append(mu_x); mean_y.append(mu_y)
            sigma_x.append(sd_x); sigma_y.append(sd_y)

# Convert to numpy arrays
true_x = np.array(true_x); true_y = np.array(true_y); true_z = np.array(true_z)
predicted_x = np.array(predicted_x); predicted_y = np.array(predicted_y); predicted_z = np.array(predicted_z)
mean_x = np.array(mean_x); mean_y = np.array(mean_y)
sigma_x = np.array(sigma_x); sigma_y = np.array(sigma_y)

# Compute deltas for the NN.
delta_x_NN = true_x - predicted_x
delta_y_NN = true_y - predicted_y
delta_z_NN = true_z - predicted_z

# Compute deltas for the classical method
delta_x_classical = true_x - pixel_size*mean_x
delta_y_classical = true_y - pixel_size*mean_y

# Histograms of (true - predicted) for x, y, and z
nbins = 50

fig, axes = plt.subplots(1, 2, figsize=(14, 4))
flat_axes = axes.ravel()
ax0, ax1 = flat_axes[0], flat_axes[1]

ax0.hist(delta_x_NN, bins=nbins, label=f"x ($\sigma$ = {np.std(delta_x_NN):.2f})", alpha=0.7)
ax0.hist(delta_y_NN, bins=nbins, label=f"y ($\sigma$ = {np.std(delta_y_NN):.2f})", alpha=0.7)
ax0.hist(delta_z_NN, bins=nbins, label=f"z ($\sigma$ = {np.std(delta_z_NN):.2f})", alpha=0.7)
ax0.set_xlabel("NN (True - Predicted) Positions",fontsize=14)
ax0.set_ylabel("Counts/bin",fontsize=14)
ax0.legend()

ax1.hist(delta_x_classical, bins=nbins, label=f"x ($\sigma$ = {np.std(delta_x_classical):.2f})", alpha=0.7)
ax1.hist(delta_y_classical, bins=nbins, label=f"y ($\sigma$ = {np.std(delta_y_classical):.2f})", alpha=0.7)
ax1.set_xlabel("Classical (True - Predicted) Positions",fontsize=14)
ax1.set_ylabel("Counts/bin",fontsize=14)
ax1.legend()

plt.show()
