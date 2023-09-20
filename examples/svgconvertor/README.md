# svgconvertor tool 

This tool is used to convert SVG or PNG file into eraw.bin which can be loaded
by EGT code directly to make graphic render fast. Usually this tool is running
on your Linux host PC, because it has no meaning if you convert graphics on
embedded target.

## Usage Example

1. Make egt/examples to be sure svgconvertor is a valid executable file on your
Linux host.
2. Start the convertor, of course, you can run ./svgconvertor -h to get help.

```
./svgconvertor -s
./svgconvertor -i png file1.png
./svgconvertor -i png file2.png
......
./svgconvertor -i png fileN.png
./svgconvertor file.svg
./svgconvertor -e
```
3. When convertion finished, copy ./eraw.h to source code to include in your
application cpp code, and copy ./eraw.bin to the target. At last your application
will read eraw.bin on target to get the graphics data.
4. The generated eraw.h has an array inidicating the graphics id with offset and
length, and it has an array index in the comment. So when you coding, you could
use the array index directly without iterating the array to find the graphics you
want. There is an example in egt/exampls/motorcycledash.cpp, please refer to that
to learn how to use the eraw.h and eraw.bin. 

## License

EGT is released under the terms of the `Apache 2` license. See the [COPYING](../COPYING)
file for more information.
