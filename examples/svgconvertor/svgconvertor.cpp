/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This software use TinyXML-2 which is released under the zlib license:
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose, including commercial applications,
 * and to alter it and redistribute it freely, subject to the following restrictions:
 * The origin of this software must not be misrepresented; you must not claim that you wrote the original software.
 * If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * This notice may not be removed or altered from any source distribution.
 *
 */
#include <egt/ui>
#include <iostream>
#include <string>
#include <cstring>
#include <cxxopts.hpp>
#include "../src/detail/erawimage.h"
#include "../src/detail/eraw.h"
#include "tinyxml2.h"

using namespace std;
using namespace egt;
using namespace tinyxml2;

/*
// #define SET_FILE(_ptr_, _filename_) do {  \
// 	_ptr_ = strrchr(_filename_, '/');  \
// 	if (_ptr_ == NULL)  \
// 		_ptr_ = _filename_;  \
// 	else  \
// 		_ptr_++;  \
// } while (0)
*/

class SVG_CVT
{
public:
	SVG_CVT(const string& svgpath, SvgImage& svg) noexcept
      : m_svg(svg),
        m_svgpath(svgpath)
	{ }

  void serializeSVG();
  void saveErawById(const string& filename, const string& id);
  void recursiveGLabel(XMLElement* g1);
  void convertNotGLabel(XMLElement* notg);
  XMLElement* queryUserNodeByName(XMLElement* root, const string& node);
  //constexpr std::uint32_t hash_str_to_uint32(const char* data);
  ~SVG_CVT() {}

private:
  SvgImage& m_svg;
  const string& m_svgpath;
};

static void SerializePNG(const char* png_src);
static void InitErawHFile(void);
static void EndErawHFile(void);
static void WriteTableIndexFile(void);
static void ReadTableIndexFile(void);

constexpr std::uint32_t hash_str_to_uint32(const char* data)
{
    std::uint32_t h(0);
    for (int i = 0; data && ('\0' != data[i]); i++)
        h = (h << 6) ^ (h >> 26) ^ data[i];
    return h;
}

typedef struct {
    string name;
    unsigned int offset = 0;
    unsigned int len = 0;
} eraw_st;

#define ERAW_NAME "eraw.bin"
unsigned int offset = 0;
unsigned int table_index = 0;

std::vector<std::shared_ptr<eraw_st>> eraw_v;

void SVG_CVT::saveErawById(const string& filename, const string& id)
{
    //eraw_st local_eraw;
    unsigned int len = 0;

    auto box = m_svg.id_box(id);
    auto layer = make_shared<Image>(m_svg.render(id, box));

    detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());
    e.save(ERAW_NAME, data, box.x(), box.y(), width, height, &len);

    //local_eraw.name = filename;
    //local_eraw.offset = offset;
    //local_eraw.len = len;
    
    std::ofstream erawmap("eraw.h", std::ios_base::app);
    if (!erawmap.is_open()) {
        cout << "eraw.cfg open ERROR!" << endl;
        return;
    }

    erawmap << "    {";
    erawmap << "\"";
    erawmap << filename.c_str();
    erawmap << "\"";
    erawmap << ", ";
    erawmap << offset;
    erawmap << ", ";
    erawmap << len;
    erawmap << "},    //";
    erawmap << table_index++;
    erawmap << "\t\n";

    // erawmap << "#define ";
    // erawmap << filename;
    // erawmap << " ";
    // erawmap << offset;
    // erawmap << "\t\n";
    // erawmap << "#define ";
    // erawmap << filename + "_LEN";
    // erawmap << " ";
    // erawmap << len;
    // erawmap << "\t\n";

    erawmap.close();
    //eraw_v.push_back(local_eraw);
    WriteTableIndexFile();
    offset += len;
}

XMLElement* SVG_CVT::queryUserNodeByName(XMLElement* root, const string& name)
{
	XMLElement* userNode = root->FirstChildElement("defs");
	while(userNode->Name() != name)
		userNode = userNode->NextSiblingElement();
	return userNode;
}

void SVG_CVT::recursiveGLabel(XMLElement* g1)
{
    string id;
    string path;
    XMLElement* svgWgt = g1->FirstChildElement();
    while (NULL != svgWgt)
    {
        switch (hash_str_to_uint32(svgWgt->Name()))
        {
            case hash_str_to_uint32("g"):
                id = svgWgt->Attribute("id");
                path = id;
                id = "#" + id;
                saveErawById(path, id);
                recursiveGLabel(svgWgt);
                break;
            case hash_str_to_uint32("rect"):
            case hash_str_to_uint32("path"):
            case hash_str_to_uint32("text"):
            case hash_str_to_uint32("image"):
            case hash_str_to_uint32("use"):
            case hash_str_to_uint32("ellipse"):
            case hash_str_to_uint32("circle"):
            case hash_str_to_uint32("line"):
            case hash_str_to_uint32("polyline"):
            case hash_str_to_uint32("polygon"):
                id = svgWgt->Attribute("id");
                path = id;
                id = "#" + id;
                saveErawById(path, id);
                break;
            default:
                break;
        }
        svgWgt = svgWgt->NextSiblingElement();
    }
}

void SVG_CVT::convertNotGLabel(XMLElement* notg)
{
    string id;
    string path;
    switch (hash_str_to_uint32(notg->Name()))
    {
        case hash_str_to_uint32("rect"):
        case hash_str_to_uint32("path"):
        case hash_str_to_uint32("text"):
        case hash_str_to_uint32("image"):
        case hash_str_to_uint32("use"):
        case hash_str_to_uint32("ellipse"):
        case hash_str_to_uint32("circle"):
        case hash_str_to_uint32("line"):
        case hash_str_to_uint32("polyline"):
        case hash_str_to_uint32("polygon"):
            id = notg->Attribute("id");
            path = id;
            id = "#" + id;
            saveErawById(path, id);
            break;
        default:
            break;
    }
}

void SVG_CVT::serializeSVG()
{
	XMLDocument doc;

	if (doc.LoadFile(m_svgpath.c_str())!=0)
	{
		cerr<<"load svg file failed"<<endl;
		return;
	}

	XMLElement* root = doc.RootElement();
	XMLElement* g1 = queryUserNodeByName(root, "g"); //Find the first 'g' label in SVG file, the starting of SVG graphics description
    while (NULL != g1)
    {
        if (!strcmp("g", g1->Name()))
        {
            recursiveGLabel(g1);
        }
        else
        {
            convertNotGLabel(g1);
        }

        g1 = g1->NextSiblingElement();
    }
}

static void SerializePNG(const char* png_src)
{
    // eraw_st local_eraw;
    unsigned int len = 0;

    egt::shared_cairo_surface_t surface;
    egt::detail::ErawImage e;
    surface =
            egt::shared_cairo_surface_t(cairo_image_surface_create_from_png(png_src),
                                        cairo_surface_destroy);
    const auto data = cairo_image_surface_get_data(surface.get());
    const auto width = cairo_image_surface_get_width(surface.get());
    const auto height = cairo_image_surface_get_height(surface.get());
    e.save(ERAW_NAME, data, 0, 0, width, height, &len);

    // local_eraw.name = png_src;
    // local_eraw.offset = offset;
    // local_eraw.len = len;
    
    // eraw_v.push_back(local_eraw);
    // offset += len;

    std::string s(png_src);
    std::string png = s.substr(0, s.length()-4);

    std::ofstream erawmap("eraw.h", std::ios_base::app);
    if (!erawmap.is_open()) {
        cout << "eraw.cfg open ERROR!" << endl;
        return;
    }

    erawmap << "    {";
    erawmap << "\"";
    erawmap << png.c_str();
    erawmap << "\"";
    erawmap << ", ";
    erawmap << offset;
    erawmap << ", ";
    erawmap << len;
    erawmap << "},    //";
    erawmap << table_index++;
    erawmap << "\t\n";

    // erawmap << "#define ";
    // erawmap << png;
    // erawmap << " ";
    // erawmap << offset;
    // erawmap << "\t\n";
    // erawmap << "#define ";
    // erawmap << png + "_LEN";
    // erawmap << " ";
    // erawmap << len;
    // erawmap << "\t\n";

    erawmap.close();
    //eraw_v.push_back(local_eraw);
    offset += len;
    WriteTableIndexFile();
}

static void WriteTableIndexFile(void) {
    std::ofstream indexfile("index.txt");
    if (!indexfile.is_open()) {
        cout << "indexfile open ERROR!" << endl;
        return;
    }
    
    indexfile << table_index << " " << offset;
    indexfile.close();
}

static void ReadTableIndexFile(void) {
    std::ifstream indexfile("index.txt", std::ios::in);
    if (!indexfile.is_open()) {
        cout << "indexfile open ERROR!" << endl;
        return;
    }
    
    indexfile >> table_index >> offset;
    indexfile.close();
}

static void InitErawHFile(void) {
    std::ofstream erawmap("eraw.h", std::ios_base::app);
    if (!erawmap.is_open()) {
        cout << "eraw.cfg open ERROR!" << endl;
        return;
    }
    
    erawmap << "typedef struct {";
    erawmap << "\t\n";
    erawmap << "    std::string name;";
    erawmap << "\t\n";
    erawmap << "    int offset;";
    erawmap << "\t\n";
    erawmap << "    int len;";
    erawmap << "\t\n";
    erawmap << "} eraw_st;";
    erawmap << "\t\n";
    erawmap << "\t\n";
    erawmap << "eraw_st offset_table[] = {";
    erawmap << "\t\n";

    erawmap.close();
}

static void EndErawHFile(void) {
    std::ofstream erawmap("eraw.h", std::ios_base::app);
    if (!erawmap.is_open()) {
        cout << "eraw.cfg open ERROR!" << endl;
        return;
    }
    
    erawmap << "};";
    erawmap << "\t\n";

    erawmap.close();
}


int main(int argc, char** argv)
{
    cxxopts::Options options("svgconvertor", "SVG format convertor");
    options.add_options()
    ("h,help", "help")
    ("s,starttoken", "add start token of eraw.h")
    ("e,endtoken", "add end token of eraw.h")
    ("i,input-format", "input format (svg, png)",
     cxxopts::value<std::string>()->default_value("svg"))
    ("positional", "SOURCE", cxxopts::value<std::vector<std::string>>())
    ;
    options.positional_help("SOURCE");

    options.parse_positional({"positional"});
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        cout << options.help() << endl;
        return 0;
    }

    if (result.count("starttoken"))
    {
        InitErawHFile();
        WriteTableIndexFile();
        cout << "Now you can convert SVG or PNG file!" << endl;
        return 0;
    }

    if (result.count("endtoken"))
    {
        EndErawHFile();
        if (-1 == system("rm index.txt"))
        {
            cerr << "rm index.txt failed, please check permission!!!" << endl;
            return 1;
        }
        cout << "Convertion finished! Copy eraw.h to source, and copy eraw.bin to target!" << endl;
        return 0;
    }

    if (result.count("positional") != 1)
    {
        cerr << options.help() << endl;
        return 1;
    }

    ReadTableIndexFile();

    auto& positional = result["positional"].as<vector<std::string>>();

    string in = positional[0];
    string filepath;

    filepath = "file:" + in;
    if (result["input-format"].as<string>() == "svg")
    {
        SvgImage svg(filepath, SizeF(0, 0));
        SVG_CVT svg_cvt(in, svg);
        svg_cvt.serializeSVG();
        cout << "SVG converted done!" << endl;
    }

    if (result["input-format"].as<string>() == "png")
    {
        // const char *pname = nullptr;
        // SET_FILE(pname, in);
        SerializePNG(in.c_str());
        cout << "PNG converted done!" << endl;
    }

    return 0;
}
