import sys
try:
    from PIL import Image
except ImportError:
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow"])
    from PIL import Image

img = Image.open('icon.png')
img.save('data/icons/icon.ico', format='ICO', sizes=[(256,256), (128,128), (64,64), (32,32), (16,16)])
print("Successfully generated data/icons/icon.ico")
