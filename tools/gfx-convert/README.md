# gfx-convert tool 

This tool is used to convert SVG or PNG/JPG/BMP file into eraw.bin which can be
loaded by EGT code directly to make graphic render fast. Usually this tool is 
running on your Linux host PC, because it has no meaning if you convert graphics
on embedded target.

## Usage Example

1. Before using this tool, please make sure that you've followed the egt build
instruction to build egt library ready.
```
	git clone --recursive https://github.com/MCHP-MPU-Solutions-SHA/egt.git -b br1.9
	cd egt
	./autogen.sh
	./configure
	make
	sudo make install
```
2. Make the gfx-convert tool.
```
	cd egt/tools/gfx-convert
	make
	sudo cp gfx-convert /usr/local/bin
```
3. Convert single file.
	- Convert a jpg file:
```
	gfx-convert -s
	gfx-convert -i img picture.jpg
	gfx-convert -e
```
	- Convert a svg file into one single object:
```
	gfx-convert -s
	gfx-convert -i img picture.svg
	gfx-convert -e
```
	- Convert a svg file to multi widgets objects:
```
	gfx-convert -s
	gfx-convert picture.svg
	gfx-convert -e
```

4. Convert multi files.
```
	gfx-convert -s
	gfx-convert -m
	gfx-convert -i img picture1.png
	gfx-convert -i img picture2.jpg
		......
	gfx-convert -i img pictureN.bmp
	gfx-convert picture.svg
	gfx-convert -e
```
Run the multi-process.sh to process multiple .png files automatically.
5. When conversion finished, copy ./eraw_define.h ./xxx_eraw.h to source code to include in your
application cpp code, and copy ./xxx_eraw.bin to the target. At last your application
will read xxx_eraw.bin on target to get the graphics data.

6. The generated xxx_eraw.h has an array inidicating the graphics id with offset and
length, and it has an array index in the comment. So when you are coding, you could
use the array index directly without iterating the array to find the graphics you
want. There is an example in egt/examples/rounddash.cpp, please refer to that
to learn how to use the xxx_eraw.h and xxx_eraw.bin. 

## License

EGT is released under the terms of the `Apache 2` license. See the [COPYING](../COPYING)
file for more information.
