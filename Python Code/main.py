import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
import random
import math

raw_red_data = [
    116394, 116496, 116534, 116440, 115964, 115684, 115678, 115754, 115801, 115830,
    115763, 115684, 115670, 115710, 115786, 115865, 115959, 116034, 116102, 116169,
    116232, 116305, 116394, 116427, 116009, 115582, 115476, 115558, 115630, 115692,
    115690, 115586, 115541, 115550, 115653, 115701, 115817, 115864, 115979, 116001,
    116127, 116153, 116274, 116342, 116369, 115851, 115431, 115301, 115428, 115483
]

raw_ir_data = [
    105863, 105901, 105945, 105795, 105612, 105566, 105598, 105629, 105655, 105665,
    105631, 105606, 105619, 105665, 105704, 105736, 105768, 105797, 105846, 105856,
    105899, 105938, 105983, 105899, 105711, 105576, 105605, 105637, 105679, 105685,
    105676, 105639, 105634, 105661, 105702, 105744, 105770, 105796, 105831, 105869,
    105903, 105944, 105989, 106042, 105925, 105711, 105602, 105636, 105665, 105688
]


class SensorSimulator:
    def __init__(self):
        self.pos = 0
        self.going_up = True
        self.breath_phase = 0.0
        self.motion_offset = 0.0
        self.drift = 0.0

    def apply_noise(self, clean_value, is_ir):
        val = float(clean_value)
        # Electronic Noise
        val += random.randint(-5, 5)

        # Drift & Motion
        if is_ir:
            self.drift = math.sin(self.breath_phase) * 30.0
            self.breath_phase += 0.05
            if random.randint(0, 299) == 5:
                self.motion_offset = random.randint(-600, 600)
            self.motion_offset *= 0.90
            if abs(self.motion_offset) < 1:
                self.motion_offset = 0

        if is_ir:
            val += self.drift
        else:
            val += (self.drift * 0.2)

        val += self.motion_offset
        if val < 0:
            val = 0
        return val

    def get_next_sample(self):
        r = self.apply_noise(raw_red_data[self.pos], False)
        i = self.apply_noise(raw_ir_data[self.pos], True)

        if self.going_up:
            self.pos += 1
            if self.pos >= 49:
                self.going_up = False
        else:
            self.pos -= 1
            if self.pos == 0:
                self.going_up = True
        return r, i


# Generating Signal
num_samples = 500
sim = SensorSimulator()
red_signal, ir_signal = [], []

for _ in range(num_samples):
    r, i = sim.get_next_sample()
    red_signal.append(r)
    ir_signal.append(i)

red_signal = np.array(red_signal)
ir_signal = np.array(ir_signal)
fs = 25.0

# Butterworth Filter
cutoff_hz = 0.5
nyquist = 0.5 * fs
b, a = signal.butter(1, cutoff_hz / nyquist, btype='high', output='ba')

ac_red = signal.lfilter(b, a, red_signal)
ac_ir = signal.lfilter(b, a, ir_signal)

processed_red = -ac_red * 1
processed_ir = -ac_ir * 4

# Level Shift
final_red = processed_red + 30000
final_ir = processed_ir + 30000

# Moving Average
window_size = 4
window = np.ones(window_size) / window_size
smooth_red = np.convolve(final_red, window, mode='same')
smooth_ir = np.convolve(final_ir, window, mode='same')
t = np.arange(0, num_samples) / fs


peaks, _ = signal.find_peaks(
    smooth_ir, distance=10, height=30100, prominence=50)
peak_diffs = np.diff(peaks)
avg_diff_samples = np.mean(peak_diffs)
#  Samples -> Seconds -> Minutes
avg_diff_seconds = avg_diff_samples / fs
# BPM calculation
bpm_calc = 60.0 / avg_diff_seconds


# Ratio of Ratios
spO2_values = []
for p in peaks:
    amp_ir = smooth_ir[p] - 30000
    amp_red = smooth_red[p] - 30000

    # R = AC_red / AC_ir
    if amp_ir > 0:
        ratio = amp_red / amp_ir
        # SpO2 = 110 - 25 * R
        spo2_val = 110 - (25 * ratio)
        spO2_values.append(spo2_val)

avg_spo2 = np.mean(spO2_values)

plt.figure(figsize=(12, 6))
start_idx = 50

plt.plot(t[start_idx:-1], smooth_ir[start_idx:-1],
         color='green', label='IR Signal (x4 Gain)')
plt.plot(t[start_idx:-1], smooth_red[start_idx:-1], color='orange',
         linestyle='--', label='Red Signal (x1 Gain)')

valid_peaks = peaks[peaks > start_idx]
plt.plot(t[valid_peaks], smooth_ir[valid_peaks], "rx", label='Peaks')

plt.title(f"Python Analysis: BPM={bpm_calc:.1f}, SpO2={avg_spo2:.1f}%")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude (Level Shifted)")
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()
