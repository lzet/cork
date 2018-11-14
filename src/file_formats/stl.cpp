// +-------------------------------------------------------------------------
// | stl.cpp
// | 
// | Author: lzet
// +-------------------------------------------------------------------------
// | Copyright (C) 2018 lzet
// | This program is free software: you can redistribute it and/or modify
// | it under the terms of the GNU General Public License as published by
// | the Free Software Foundation, either version 3 of the License, or
// | (at your option) any later version.
// |
// | This program is distributed in the hope that it will be useful,
// | but WITHOUT ANY WARRANTY; without even the implied warranty of
// | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// | GNU General Public License for more details.
// |
// | You should have received a copy of the GNU General Public License
// | along with this program.  If not, see <https://www.gnu.org/licenses/>.
// +-------------------------------------------------------------------------

#include "files.h"

#include <iostream>
#include <fstream>
#include <cstring>


namespace Files {

void err(const std::string &msg) {
  std::cerr << "Error: " << msg << std::endl;
}
union Vec3dBin
{
    float fVal[3];
    char  buf[12];
};

// > https://en.wikipedia.org/wiki/STL_(file_format)
// >> Binary STL
// >>> UINT8[80] – Header
// >>> UINT32 – Number of triangles
// >>>
// >>> foreach triangle
// >>> REAL32[3] – Normal vector
// >>> REAL32[3] – Vertex 1
// >>> REAL32[3] – Vertex 2
// >>> REAL32[3] – Vertex 3
// >>> UINT16 – Attribute byte count

#define FLOAT_EQ 0.0000001
bool veq(const Vec3d &v1, const Vec3d &v2) {
    return std::abs(v1.x - v2.x) < FLOAT_EQ && std::abs(v1.y - v2.y) < FLOAT_EQ && std::abs(v1.z - v2.z) < FLOAT_EQ;
}

int readSTL(std::string filename, FileMesh *data)
{
    if(!data) { err("no data"); return 1; }
    
    std::ifstream in;
    in.open(filename.c_str());
    if(!in) { err("no input data"); return 1; }

    char header[80];
    in.read(header, 80);
    if(std::strncmp("solid", header, 5) == 0) { err("not binary STL (text format)"); return 1; }

    uint32_t numvertices = 0, numfaces = 0;
    in.read(reinterpret_cast<char*>(&numfaces), 4);
    numvertices = 3*numfaces;
    data->triangles.resize(numfaces);
    if(numvertices == 0 || numfaces == 0) { err("no triangle count"); return 1; }

    std::vector<Vec3d> vertices;
    vertices.resize(numvertices);
    Vec3dBin vecbuf;

    for(size_t i = 0; i < numfaces; ++i) {
        auto &v = data->triangles[i];
        in.read(vecbuf.buf, 12);
        v.normal.x = static_cast<double>(vecbuf.fVal[0]);
        v.normal.y = static_cast<double>(vecbuf.fVal[1]);
        v.normal.z = static_cast<double>(vecbuf.fVal[2]);
        for(size_t j = 0; j < 3; ++j) {
          size_t next = i*3 + j;
          auto &p = vertices[next];
          in.read(vecbuf.buf, 12);
          p.x = static_cast<double>(vecbuf.fVal[0]);
          p.y = static_cast<double>(vecbuf.fVal[1]);
          p.z = static_cast<double>(vecbuf.fVal[2]);
        }
        uint16_t attr = 0;
        in.read(reinterpret_cast<char*>(&attr), 2);
        v.a = static_cast<int>(i*3);
        v.b = static_cast<int>(i*3+1);
        v.c = static_cast<int>(i*3+2);
    }

    // delete repeated vertices
    std::vector<Vec3d> vertices_clear;
    std::vector<int> newindx;
    for(size_t i = 0; i < numvertices; ++i) {
        auto &curv = vertices[i];
        bool has = false;
        size_t curvidx = 0;
        for(; curvidx < vertices_clear.size(); ++curvidx) {
            if(veq(curv, vertices_clear[curvidx])) {
                has = true;
                break;
            }
        }
        if(!has) {
            vertices_clear.push_back(curv);
            curvidx = vertices_clear.size() - 1;
        }
        newindx.emplace_back(static_cast<int>(curvidx));
    }
    for(auto &t: data->triangles) {
        t.a = newindx[t.a];
        t.b = newindx[t.b];
        t.c = newindx[t.c];
        //err("triangle a:"+std::to_string(t.a)+" b:"+std::to_string(t.b)+" c:"+std::to_string(t.c));
    }
    if(vertices_clear.size() < 3) { err("no vertices ("+std::to_string(vertices_clear.size())+")"); return 1; }

    // fill vertices
    data->vertices.resize(vertices_clear.size());
    for(size_t i = 0; i < vertices_clear.size(); ++i) {
        auto &tov = data->vertices[i].pos;
        auto &frv = vertices_clear[i];
        tov.x = frv.x;
        tov.y = frv.y;
        tov.z = frv.z;
    }
    err("was v:"+std::to_string(vertices.size())+" now v:"+std::to_string(vertices_clear.size()));

    return 0;
}

int writeSTL(std::string filename, FileMesh *data)
{
    if(!data) return 1;

    std::ofstream out;
    std::string header(filename);
    header += " cork STL binary";
    for(int i = 80-header.size(); i >= 0; --i)
      header += ' ';
    out.open(filename.c_str());
    if(!out) return 1;
    out.write(header.c_str(), 80);
    uint32_t numvertices = data->vertices.size();
    uint32_t numfaces = data->triangles.size();
    out.write(reinterpret_cast<char*>(&numfaces), 4);
    Vec3dBin vecbuf;
    for(size_t i = 0; i < numfaces; ++i) {
        vecbuf.fVal[0] = 0; // TODO: add normal vector
        vecbuf.fVal[1] = 0;
        vecbuf.fVal[2] = 0;
        out.write(vecbuf.buf, 12);
        auto &tri = data->triangles[i];

        auto &vert1 = data->vertices[tri.a];
        auto &vert2 = data->vertices[tri.b];
        auto &vert3 = data->vertices[tri.c];

        vecbuf.fVal[0] = vert1.pos.x;
        vecbuf.fVal[1] = vert1.pos.y;
        vecbuf.fVal[2] = vert1.pos.z;
        out.write(vecbuf.buf, 12);
        vecbuf.fVal[0] = vert2.pos.x;
        vecbuf.fVal[1] = vert2.pos.y;
        vecbuf.fVal[2] = vert2.pos.z;
        out.write(vecbuf.buf, 12);
        vecbuf.fVal[0] = vert3.pos.x;
        vecbuf.fVal[1] = vert3.pos.y;
        vecbuf.fVal[2] = vert3.pos.z;
        out.write(vecbuf.buf, 12);

        uint16_t attr = 0;
        out.write(reinterpret_cast<char*>(&attr), 2);
    }

    if(!out) return 1;

    return 0;
}



} // end namespace Files
