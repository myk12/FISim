import os
import pandas as pd
import matplotlib.pyplot as plt

time_interval = 0.07

#first process the results with ID First Routing off
output_dir_off = "/tmp/mobility-test/Routing_Off"
df_off_1 = pd.read_csv(output_dir_off + "/receiver_2.log", sep=",")
df_off_2 = pd.read_csv(output_dir_off + "/receiver_3.log", sep=",")
df_off = pd.concat([df_off_1, df_off_2], ignore_index=True)
df_off_plt = df_off.groupby(pd.cut(df_off["time"], bins=pd.interval_range(start=0, end=df_off['time'].max(), freq=time_interval)))['packet_size'].sum().reset_index()

#then process the results with ID First Routing on
output_dir_on = "/tmp/mobility-test/Routing_On"
df_on_1 = pd.read_csv(output_dir_on + "/receiver_2.log", sep=",")
df_on_2 = pd.read_csv(output_dir_on + "/receiver_3.log", sep=",")
df_on = pd.concat([df_on_1, df_on_2], ignore_index=True)
df_on_plt = df_on.groupby(pd.cut(df_on["time"], bins=pd.interval_range(start=0, end=df_on['time'].max(), freq=time_interval)))['packet_size'].sum().reset_index()

df_off_plt['time'] = df_off_plt['time'].apply(lambda x: x.left)
df_on_plt['time'] = df_on_plt['time'].apply(lambda x: x.left)
df_off_plt['Throughput'] = df_off_plt["packet_size"] * 8 / 1000000 / time_interval
df_on_plt['Throughput'] = df_on_plt["packet_size"] * 8 / 1000000 / time_interval

print(df_on_plt)
print(df_off_plt)
# save time, throughput to csv file
df_off_plt.to_csv("mobility_test_off.csv", index=False)
df_on_plt.to_csv("mobility_test_on.csv", index=False)

# plot the results
plt.plot(df_off_plt.index * time_interval, df_off_plt["packet_size"] * 8 / 1000000 / time_interval, label="ID First Routing Off", color="black", linestyle="solid", linewidth=2)
plt.plot(df_on_plt.index * time_interval, df_on_plt["packet_size"] * 8 / 1000000 / time_interval, label="ID First Routing On", color="blue", linestyle="--", linewidth=2)
plt.xlabel("Time (s)")
plt.ylabel("Throughput (Mbps)")
plt.grid(True)

plt.legend()
plt.savefig("mobility_test.png")