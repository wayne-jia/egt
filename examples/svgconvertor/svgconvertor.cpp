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

static void SerializePNG(const char* png_src, const std::string& png_dst);

constexpr std::uint32_t hash_str_to_uint32(const char* data)
{
    std::uint32_t h(0);
    for (int i = 0; data && ('\0' != data[i]); i++)
        h = (h << 6) ^ (h >> 26) ^ data[i];
    return h;
}

void SVG_CVT::saveErawById(const string& filename, const string& id)
{
    auto box = m_svg.id_box(id);
    auto layer = make_shared<Image>(m_svg.render(id, box));

    detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());
    e.save(filename, data, box.x(), box.y(), width, height);
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
                path = "./eraw/" + id + ".eraw";
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
                path = "./eraw/" + id + ".eraw";
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
            path = "./eraw/" + id + ".eraw";
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

    if (access("./eraw", F_OK))
    {
        if (0 > mkdir("./eraw", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            cerr << "Create serialization dir eraw failed, please check permission!!!" << endl;
            return;
        }
    }
    else
    {
        if (-1 == system("rm -rf ./eraw/*"))
        {
            cerr << "rm -rf ./eraw/* failed, please check permission!!!" << endl;
            return;
        }
    }

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

static void SerializePNG(const char* png_src, const std::string& png_dst)
{
    egt::shared_cairo_surface_t surface;
    egt::detail::ErawImage e;
    surface =
            egt::shared_cairo_surface_t(cairo_image_surface_create_from_png(png_src),
                                        cairo_surface_destroy);
    const auto data = cairo_image_surface_get_data(surface.get());
    const auto width = cairo_image_surface_get_width(surface.get());
    const auto height = cairo_image_surface_get_height(surface.get());
    e.save(png_dst, data, 0, 0, width, height);
}

int main(int argc, char** argv)
{
    cxxopts::Options options("svgconvertor", "SVG format convertor");
    options.add_options()
    ("h,help", "help")
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

    if (result.count("positional") != 1)
    {
        cerr << options.help() << endl;
        return 1;
    }

    auto& positional = result["positional"].as<vector<std::string>>();

    string in = positional[0];
    string filepath;

    filepath = "file:" + in;
    if (result["input-format"].as<string>() == "svg")
    {
        SvgImage svg(filepath, SizeF(0, 0));
        SVG_CVT svg_cvt(in, svg);
        svg_cvt.serializeSVG();
    }

    if (result["input-format"].as<string>() == "png")
    {
        // const char *pname = nullptr;
        // SET_FILE(pname, in);
        SerializePNG(in.c_str(), "png.eraw");
        cout << "PNG converted OK!" << endl;
    }

    return 0;
}
