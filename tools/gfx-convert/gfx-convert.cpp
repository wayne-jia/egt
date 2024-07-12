/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <egt/ui>
#include <iostream>
#include <string>
#include <cstring>
#include <dirent.h>
#include <cxxopts.hpp>
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include <erawimage.h>
#include "../../include/egt/detail/image.h"




#define GFXCVT_VER "gfxconvert v1.2.0"


using namespace std;
using namespace egt;

class SVG_CVT
{
public:
	SVG_CVT(const string& svgname, SvgImage& svgimg) noexcept
      : m_svgname(svgname), m_svgimg(svgimg)
	{}

    void serializeSVG(void);
    void saveErawById(const string& filename, const string& id);
    void recursiveGLabel(rapidxml::xml_node<>* g1);
    void convertNotGLabel(rapidxml::xml_node<>* notg);

private:
    const string& m_svgname;
    SvgImage& m_svgimg;
};

class IMG_CVT
{
public:
	IMG_CVT(const string& imgname) noexcept
      : m_imgname(imgname)
	{}

    void serializeIMG(void);

private:
    const string& m_imgname;
};


static void EndErawHFile(void);
static void WriteTableIndexFile(void);
static void ReadTableIndexFile(void);
static void WriteHeaderIndicationFile(void);
static string GetStringB4Dot(string input);

constexpr uint32_t hash_str_to_uint32(const char* data)
{
    uint32_t h(0);
    for (int i = 0; data && ('\0' != data[i]); i++)
        h = (h << 6) ^ (h >> 26) ^ data[i];
    return h;
}


unsigned int offset = 0;
unsigned int table_index = 0;
unsigned int header_init = 0;


void SVG_CVT::saveErawById(const string& filename, const string& id)
{
    unsigned int len = 0;
    auto box = m_svgimg.id_box(id);
    auto layer = make_shared<Image>(m_svgimg.render(id, box));

    string eraw_bin_name = GetStringB4Dot(m_svgname) + "_eraw.bin";
    string eraw_h_name = GetStringB4Dot(m_svgname) + "_eraw.h";

    if (!access("multiple.txt", F_OK))
    {
        eraw_bin_name = "multiple_eraw.bin";
        eraw_h_name = "multiple_eraw.h";
    }

    detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());
    e.save(eraw_bin_name, data, box.x(), box.y(), width, height, &len);
 
    ofstream erawmap(eraw_h_name, ios_base::app);
    if (!erawmap.is_open()) {
        cerr << eraw_h_name << " open ERROR!" << endl;
        return;
    }

    if (!header_init) {
        erawmap << "#include \"eraw_define.h\"";
        erawmap << "\t\n";
        erawmap << "\t\n";
        erawmap << "\t\n";
        erawmap << "eraw_st " + GetStringB4Dot(m_svgname) + "_table[] = {";
        erawmap << "\t\n";
        header_init = 1;
        WriteHeaderIndicationFile();
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
    erawmap.close();
    WriteTableIndexFile();
    offset += len;
}

void SVG_CVT::recursiveGLabel(rapidxml::xml_node<>* g)
{
    rapidxml::xml_attribute<>* id_attr;
    string id;
    string path;
    auto svgWgt = g->first_node();

    while (NULL != svgWgt)
    {
        switch (hash_str_to_uint32(svgWgt->name()))
        {
            case hash_str_to_uint32("g"):
                id_attr = svgWgt->first_attribute("id");
                id = id_attr->value();
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
                id_attr = svgWgt->first_attribute("id");
                id = id_attr->value();
                path = id;
                id = "#" + id;
                saveErawById(path, id);
                break;

            default:
                break;
        }

        svgWgt = svgWgt->next_sibling();
    }
}

void SVG_CVT::convertNotGLabel(rapidxml::xml_node<>* notg)
{
    rapidxml::xml_attribute<>* id_attr;
    string id;
    string path;

    switch (hash_str_to_uint32(notg->name()))
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
            id_attr = notg->first_attribute("id");
            id = id_attr->value();
            path = id;
            id = "#" + id;
            saveErawById(path, id);
            break;

        default:
            break;
    }
}

void SVG_CVT::serializeSVG(void)
{
    rapidxml::file<> xml_file(m_svgname.c_str());
    rapidxml::xml_document<> doc;
    doc.parse<0>(xml_file.data());

    auto svg = doc.first_node("svg");
    if (svg)
    {
        auto g = svg->first_node("g");
	    while (NULL != g)
        {
            if (!strcmp("g", g->name()))
                recursiveGLabel(g);
            else
                convertNotGLabel(g);

            g = g->next_sibling();
        }
    }
    else
    {
        cerr << "file format is incorrect" << endl;
		return;
    }
}

static string GetStringB4Dot(string input)
{
    // Find the position of the first dot
    size_t dotPosition = input.find('.');

    if (dotPosition != string::npos) {
        // Remove the part after the dot
        string result = input.substr(0, dotPosition);
        
        return result;
    } else {
        // The string does not contain a dot
        cerr << "String does not contain a dot." << endl;
        return input;
    }
}

void IMG_CVT::serializeIMG(void)
{
    unsigned int len = 0;

    string eraw_bin_name = GetStringB4Dot(m_imgname) + "_eraw.bin";
    string eraw_h_name = GetStringB4Dot(m_imgname) + "_eraw.h";

    if (!access("multiple.txt", F_OK))
    {
        eraw_bin_name = "multiple_eraw.bin";
        eraw_h_name = "multiple_eraw.h";
    }

    shared_cairo_surface_t surface;
    detail::ErawImage e;
    surface = detail::load_image_from_filesystem(m_imgname);
    const auto data = cairo_image_surface_get_data(surface.get());
    const auto width = cairo_image_surface_get_width(surface.get());
    const auto height = cairo_image_surface_get_height(surface.get());
    e.save(eraw_bin_name, data, 0, 0, width, height, &len);

    ofstream erawmap(eraw_h_name, ios_base::app);
    if (!erawmap.is_open()) 
    {
        cerr << eraw_h_name << " open ERROR!" << endl;
        return;
    }

    if (!header_init) {
        erawmap << "#include \"eraw_define.h\"";
        erawmap << "\t\n";
        erawmap << "\t\n";
        erawmap << "\t\n";
        erawmap << "eraw_st " + GetStringB4Dot(m_imgname) + "_table[] = {";
        erawmap << "\t\n";
        header_init = 1;
        WriteHeaderIndicationFile();
    }

    erawmap << "    {";
    erawmap << "\"";
    erawmap << GetStringB4Dot(m_imgname).c_str();
    erawmap << "\"";
    erawmap << ", ";
    erawmap << offset;
    erawmap << ", ";
    erawmap << len;
    erawmap << "},    //";
    erawmap << table_index++;
    erawmap << "\t\n";
    erawmap.close();
    offset += len;
    WriteTableIndexFile();
}

static void WritePulicHeadFile(void) 
{
    ofstream eraw_def_file("eraw_define.h");
    if (!eraw_def_file.is_open()) 
    {
        cerr << "eraw_def_file open ERROR!" << endl;
        return;
    }
    
    eraw_def_file << "#ifndef __ERAW_DEFINE_H__\t\n";
    eraw_def_file << "#define __ERAW_DEFINE_H__\t\n";
    eraw_def_file << "\t\n";
    eraw_def_file << "\t\n";
    eraw_def_file << "\t\n";
    eraw_def_file << "#include <string>\t\n";
    eraw_def_file << "\t\n";
    eraw_def_file << "\t\n";
    eraw_def_file << "typedef struct {\t\n";
    eraw_def_file << "    std::string name;\t\n";
    eraw_def_file << "    int offset;\t\n";
    eraw_def_file << "    int len;\t\n";
    eraw_def_file << "} eraw_st;\t\n";
    eraw_def_file << "\t\n";
    eraw_def_file << "#endif\t\n";
    eraw_def_file.close();
}

static void WriteTableIndexFile(void) 
{
    ofstream indexfile("index.txt");
    if (!indexfile.is_open()) 
    {
        cerr << "indexfile open ERROR!" << endl;
        return;
    }
    
    indexfile << table_index << " " << offset;
    indexfile.close();
}

static void ReadTableIndexFile(void) 
{
    ifstream indexfile("index.txt", ios::in);
    if (!indexfile.is_open()) 
    {
        cerr << "indexfile open ERROR!" << endl;
        return;
    }
    
    indexfile >> table_index >> offset;
    indexfile.close();
}

static void WriteHeaderIndicationFile() 
{
    ofstream headerfile("header.txt");
    if (!headerfile.is_open()) 
    {
        cerr << "headerfile open ERROR!" << endl;
        return;
    }
    
    headerfile << header_init;
    headerfile.close();
}

static void ReadHeaderIndicationFile(void) 
{
    ifstream headerfile("header.txt", ios::in);
    if (!headerfile.is_open()) 
    {
        cerr << "headerfile open ERROR!" << endl;
        return;
    }
    
    headerfile >> header_init;
    headerfile.close();
}

static void WriteMultipleNameFile(void) 
{
    ofstream multiplefile("multiple.txt");
    if (!multiplefile.is_open()) 
    {
        cerr << "multiplefile open ERROR!" << endl;
        return;
    }
    
    multiplefile << "multiple";
    multiplefile.close();
}

static void EndErawHFile(void) 
{
    string eraw_h_name = "";
    if (!access("multiple.txt", F_OK))
    {
        eraw_h_name = "multiple_eraw.h";
    }
    else
    {
        DIR *dir;
        struct dirent *ent;

        if ((dir = opendir("./")) != nullptr) {
            while ((ent = readdir(dir)) != nullptr) {
                std::string filename = ent->d_name;

                // Check if the file ends with ".h"
                if (filename.length() > 6 && filename.substr(filename.length() - 6) == "eraw.h") {
                    eraw_h_name = filename;
                    break;
                }
            }
            closedir(dir);
        } else {
            cerr << "Error opening directory: ./" << endl;
        }
    }

    if (eraw_h_name == "")
        return;

    ofstream erawmap(eraw_h_name, ios_base::app);
    if (!erawmap.is_open()) 
    {
        cerr << eraw_h_name <<" open ERROR!" << endl;
        return;
    }
    
    erawmap << "};";
    erawmap.close();
}


int main(int argc, char** argv)
{
    cxxopts::Options options("gfxconvert", "SVG and Image(png,jpg,bmp) format convertor");
    options.add_options()
    ("h,help", "Usage: 1 ./gfxconvert -s 2 ./gfxconvert -i img abc.png 3 ./gfxconvert -i img abc.jpg 4 ./gfxconvert -e")
    ("s,starttoken", "start token of gfxconvert")
    ("e,endtoken", "end token of gfxconvert")
    ("m,multiple", "convert multiple files(png,jpg,bmp,svg) into one file 'multimple.bin'")
    ("i,input-format", "input format (svg, img) img include png/jpg/bmp",
     cxxopts::value<string>()->default_value("svg"))
    ("positional", "SOURCE", cxxopts::value<vector<string>>())
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
        WritePulicHeadFile();
        WriteTableIndexFile();
        WriteHeaderIndicationFile();
        cout << GFXCVT_VER << endl << "Now you can convert SVG or IMG(png,jpg,bmp) file!" << endl;
        return 0;
    }

    if (result.count("endtoken"))
    {
        EndErawHFile();
        if (-1 == system("rm *.txt"))
        {
            cerr << "rm *.txt failed, please check permission!!!" << endl;
            return 1;
        }
        cout << "Conversion finished! Copy *.h to source, and copy *.bin to target!" << endl;
        return 0;
    }

    if (result.count("multiple"))
    {
        WriteMultipleNameFile();
        return 0;
    }

    if (result.count("positional") != 1)
    {
        cerr << options.help() << endl;
        return 1;
    }

    ReadTableIndexFile();
    ReadHeaderIndicationFile();

    auto& positional = result["positional"].as<vector<string>>();

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

    if (result["input-format"].as<string>() == "img")
    {
        IMG_CVT img_cvt(in);
        img_cvt.serializeIMG();
        cout << "IMG converted done!" << endl;
    }

    return 0;
}
