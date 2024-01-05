# gfxconvert tool 

This tool is used to convert SVG or PNG/JPG/BMP file into eraw.bin which can be
loaded by EGT code directly to make graphic render fast. Usually this tool is 
running on your Linux host PC, because it has no meaning if you convert graphics
on embedded target.

## Usage Example

1. Before using this tool, please make sure that you've followed the egt build
instruction to build egt library ready.
```
	git clone --recursive https://github.com/MCHP-MPU-Solutions-SHA/egt.git -b br1.8
	cd egt
	./autogen.sh
	./configure
	make
	sudo make install
```
2. Make the gfxconvert tool.
```
	cd egt/tool/gfxconvert
	make
	sudo cp gfxconvert /usr/local/bin
```
3. Convert single file.
	- Convert a jpg file:
```
	gfxconvert -s
	gfxconvert -i img picture.jpg
	gfxconvert -e
```
	- Convert a svg file into one single object:
```
	gfxconvert -s
	gfxconvert -i img picture.svg
	gfxconvert -e
```
	- Convert a svg file to multi widgets objects:
```
	gfxconvert -s
	gfxconvert picture.svg
	gfxconvert -e
```

4. Convert multi files.
```
	gfxconvert -s
	gfxconvert -m
	gfxconvert -i img picture1.png
	gfxconvert -i img picture2.jpg
		......
	gfxconvert -i img pictureN.bmp
	gfxconvert picture.svg
	gfxconvert -e
```
5. When conversion finished, copy ./filename_eraw.h to source code to include in your
application cpp code, and copy ./filename_eraw.bin to the target. At last your application
will read filename_eraw.bin on target to get the graphics data.

6. The generated filename_eraw.h has an array inidicating the graphics id with offset and
length, and it has an array index in the comment. So when you are coding, you could
use the array index directly without iterating the array to find the graphics you
want. There is an example in egt/examples/motorcycledash.cpp, please refer to that
to learn how to use the filename_eraw.h and filename_eraw.bin. 

## License

EGT is released under the terms of the `Apache 2` license. See the [COPYING](../COPYING)
file for more information.
