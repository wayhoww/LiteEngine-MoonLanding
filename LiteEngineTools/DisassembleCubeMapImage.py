from PIL import Image
import numpy as np
import os 
import sys

path = sys.argv[1]

image = np.array(Image.open(path))

(height, width, channel) = image.shape

I0 = 0
I1 = round(height * 1 / 3)
I2 = round(height * 2 / 3)
I3 = height


J0 = 0
J1 = round(width * 1 / 4)
J2 = round(width * 2 / 4)
J3 = round(width * 3 / 4)
J4 = width


# https://docs.microsoft.com/en-us/windows/win32/direct3d9/cubic-environment-mapping
images = {}
images[2] = image[I0:I1, J1:J2]
images[1] = image[I1:I2, J0:J1]
images[4] = image[I1:I2, J1:J2]
images[0] = image[I1:I2, J2:J3]
images[5] = image[I1:I2, J3:J4]
images[3] = image[I2:I3, J1:J2]

dirname = os.path.join(os.path.dirname(path), os.path.basename(path) + ".cubemap")
try:
	os.mkdir(dirname)
except:
	pass

assert(channel == 3 or channel == 4)
if channel == 3:
	FORMAT = "RGB"
else:
	FORMAT = "RGBA"

paths = [os.path.join(dirname, str(i) + ".png") for i in range(6)]

for i in range(6):
	out = Image.fromarray(images[i].astype('uint8'), FORMAT)
	out.save(paths[i])

print("\nImages have been generated.\n")
print("texassemble cube", " ".join(paths))
print("\nInvoke the above command to generate `dds` file.")
print("See https://github.com/Microsoft/DirectXTex/wiki/Texassemble")