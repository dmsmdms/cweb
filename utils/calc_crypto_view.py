#!/bin/python

import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file
df = pd.read_csv("../train/btcusdt2.csv")

# Convert the date column to datetime type (important for time series)
df['timestamp'] = pd.to_datetime(df['timestamp'])

# Plot the base price line
plt.figure(figsize=(12, 6))
plt.plot(df['timestamp'], df['price'], label='Price', color='blue')

# Grab only the "label == 1" points
grow_points2 = df[df['label_2'] == 1]
grow_points = df[df['label'] == 1]

# Mark them on the chart
plt.scatter(grow_points2['timestamp'], grow_points2['price'], color='green', marker='^', s=50, label='Grow Signal (2)')
plt.scatter(grow_points['timestamp'], grow_points['price'], color='red', marker='^', s=100, label='Grow Signal')

plt.title("Crypto Price with Growth Signals")
plt.xlabel("Time")
plt.ylabel("Price (USD)")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
