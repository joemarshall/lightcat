import subprocess,sys
from pathlib import Path

Import("env")
try:
    import png,lz4
except:    
    env.Execute("$PYTHONEXE -m pip install pypng lz4")

args=[sys.executable,"LVGLImage.py","images","--ofmt","C","--cf","RGB565A8","-o","src/images"]
print(args)
subprocess.check_call(args)

for x in Path("src/images").glob("*.c"):
    print(x) 