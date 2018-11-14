// +-------------------------------------------------------------------------
// | main.cpp
// | 
// | Author: Gilbert Bernstein
// +-------------------------------------------------------------------------
// | COPYRIGHT:
// |    Copyright Gilbert Bernstein 2013
// |    See the included COPYRIGHT file for further details.
// |    
// |    This file is part of the Cork library.
// |
// |    Cork is free software: you can redistribute it and/or modify
// |    it under the terms of the GNU Lesser General Public License as
// |    published by the Free Software Foundation, either version 3 of
// |    the License, or (at your option) any later version.
// |
// |    Cork is distributed in the hope that it will be useful,
// |    but WITHOUT ANY WARRANTY; without even the implied warranty of
// |    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// |    GNU Lesser General Public License for more details.
// |
// |    You should have received a copy 
// |    of the GNU Lesser General Public License
// |    along with Cork.  If not, see <http://www.gnu.org/licenses/>.
// +-------------------------------------------------------------------------

// This file contains a command line program that can be used
// to exercise Cork's functionality without having to write
// any code.

#include "files.h"
#include <cstring>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <sstream>
using std::stringstream;
using std::string;

using std::ostream;

#include "cork.h"

bool tricmp(const Files::FileTriangle &f1, const Files::FileTriangle &f2) {
    std::vector<int> cmp1 = {f1.a, f1.b, f1.c};
    std::vector<int> cmp2 = {f2.a, f2.b, f2.c};
    for(const auto &c: cmp1) {
        for(auto i = cmp2.begin(); i != cmp2.end(); ++i) {
            if(*i == c) {
                cmp2.erase(i);
                break;
            }
        }
    }
    if(cmp2.empty()) std::cerr << "tricmp: duplicate vertice error" << std::endl;
    if(cmp2.size() < 2) return true;
    return false;
}

int addvertex(int oldindex, const std::vector<Files::FileVertex> &fullvertices, std::vector<std::pair<const Files::FileVertex*, int>> &localvertices) {
    for(int i = 0; i < localvertices.size(); ++i) {
        if(localvertices[i].second == oldindex)
            return i;
    }
    localvertices.emplace_back(&fullvertices[oldindex], oldindex);
    return localvertices.size()-1;
}

void file2corktrimesh(
    const Files::FileMesh &in, std::vector<CorkTriMesh> &outv, bool needsplit = false
) {
    if(!needsplit) {
        outv.emplace_back();
        auto out = outv.end()-1;
        out->n_vertices = in.vertices.size();
        out->n_triangles = in.triangles.size();

        out->triangles = new uint[(out->n_triangles) * 3];
        out->vertices  = new float[(out->n_vertices) * 3];

        for(uint i=0; i<out->n_triangles; i++) {
            (out->triangles)[3*i+0] = in.triangles[i].a;
            (out->triangles)[3*i+1] = in.triangles[i].b;
            (out->triangles)[3*i+2] = in.triangles[i].c;
        }

        for(uint i=0; i<out->n_vertices; i++) {
            (out->vertices)[3*i+0] = in.vertices[i].pos.x;
            (out->vertices)[3*i+1] = in.vertices[i].pos.y;
            (out->vertices)[3*i+2] = in.vertices[i].pos.z;
        }
    }
    else {
        std::vector<std::vector<Files::FileTriangle>> sorted = {std::vector<Files::FileTriangle>()};
        auto triangles = in.triangles;
        while(!triangles.empty()) {
            size_t size = triangles.size();
            std::vector<std::vector<Files::FileTriangle>::iterator> fordel;
            for(auto tri = triangles.begin(); tri != triangles.end(); ++tri) {
                auto &cursorted = sorted.back();
                if(cursorted.empty()) {
                    cursorted.push_back(*tri);
                    fordel.push_back(tri);
                }
                else {
                    for(const auto &t: cursorted) {
                        if(tricmp(t, *tri)) {
                            cursorted.push_back(*tri);
                            fordel.push_back(tri);
                            break;
                        }
                    }
                }
            }
            for(auto &v: fordel)
                triangles.erase(v);
            if(size == triangles.size()) {
                sorted.push_back(std::vector<Files::FileTriangle>());
            }
        }
        /*for(uint i=0; i<in.triangles.size(); ++i) {
            auto &tri = in.triangles[i];
            bool finded = false;
            for(auto &tpart: sorted) {
                for(const auto &t: tpart) {
                    if(tricmp(t, tri)) {
                        tpart.emplace_back(tri);
                        finded = true;
                        break;
                    }
                }
                if(finded) break;
            }
            if(!finded) {
                std::vector<Files::FileTriangle> newpart;
                newpart.emplace_back(tri);
                sorted.push_back(std::move(newpart));
            }
        }*/
        std::cout << "Split file: " << sorted.size() << " objects" << std::endl;
        int i = 0;
        for(auto &s: sorted) {
            ++i;
            outv.emplace_back();
            auto out = outv.end()-1;
            out->n_triangles = s.size();
            std::vector<std::pair<const Files::FileVertex*, int>> localvertices;
            out->triangles = new uint[(out->n_triangles) * 3];
            for(uint i=0; i<out->n_triangles; i++) {
                (out->triangles)[3*i+0] = addvertex(in.triangles[i].a, in.vertices, localvertices);
                (out->triangles)[3*i+1] = addvertex(in.triangles[i].b, in.vertices, localvertices);
                (out->triangles)[3*i+2] = addvertex(in.triangles[i].c, in.vertices, localvertices);
            }
            out->n_vertices = localvertices.size();
            out->vertices  = new float[(out->n_vertices) * 3];
            for(uint i=0; i<out->n_vertices; i++) {
                (out->vertices)[3*i+0] = localvertices[i].first->pos.x;
                (out->vertices)[3*i+1] = localvertices[i].first->pos.y;
                (out->vertices)[3*i+2] = localvertices[i].first->pos.z;
            }
            std::cout << "Object" << i << " :" << out->n_triangles << "/" << out->n_vertices << std::endl;
        }
    }
}

void corktrimesh2file(
    CorkTriMesh in, Files::FileMesh &out
) {
    out.vertices.resize(in.n_vertices);
    out.triangles.resize(in.n_triangles);
    
    for(uint i=0; i<in.n_triangles; i++) {
        out.triangles[i].a = in.triangles[3*i+0];
        out.triangles[i].b = in.triangles[3*i+1];
        out.triangles[i].c = in.triangles[3*i+2];
    }
    
    for(uint i=0; i<in.n_vertices; i++) {
        out.vertices[i].pos.x = in.vertices[3*i+0];
        out.vertices[i].pos.y = in.vertices[3*i+1];
        out.vertices[i].pos.z = in.vertices[3*i+2];
    }
}

void loadMesh(string filename, std::vector<CorkTriMesh> &outv, bool needsplit = false)
{
    Files::FileMesh filemesh;
    int r = Files::readTriMesh(filename, &filemesh);
    if(r > 0) {
        cerr << "Unable to load in " << filename << endl;
        exit(1);
    }
    file2corktrimesh(filemesh, outv, needsplit);
}
void saveMesh(string filename, CorkTriMesh in)
{
    Files::FileMesh filemesh;
    
    corktrimesh2file(in, filemesh);
    
    if(Files::writeTriMesh(filename, &filemesh) > 0) {
        cerr << "Unable to write to " << filename << endl;
        exit(1);
    }
}



class CmdList {
public:
    CmdList();
    ~CmdList() {}
    
    void regCmd(
        string name,
        string helptxt,
        std::function< void(std::vector<string>::iterator &,
                            const std::vector<string>::iterator &) > body
    );
    
    void printHelp(ostream &out);
    void runCommands(std::vector<string>::iterator &arg_it,
                     const std::vector<string>::iterator &end_it);
    
private:
    struct Command {
        string name;    // e.g. "show" will be invoked with option "-show"
        string helptxt; // lines to be displayed
        std::function< void(std::vector<string>::iterator &,
                       const std::vector<string>::iterator &) > body;
    };
    std::vector<Command> commands;
};

CmdList::CmdList()
{
    regCmd("help",
    "-help                               show this help message",
    [this](std::vector<string>::iterator &,
           const std::vector<string>::iterator &) {
        printHelp(cout);
        exit(0);
    });
}

void CmdList::regCmd(
    string name,
    string helptxt,
    std::function< void(std::vector<string>::iterator &,
                        const std::vector<string>::iterator &) > body
) {
    Command cmd = {
        name,
        helptxt,
        body
    };
    commands.push_back(cmd);
}

void CmdList::printHelp(ostream &out)
{
    out <<
    "Welcome to Cork.  Usage:" << endl <<
    "  > cork [-command arg0 arg1 ... argn]*" << endl <<
    "for example," << endl <<
    "  > cork -union box0.off box1.off result.off" << endl <<
    "Options:" << endl;
    for(auto &cmd : commands)
        out << cmd.helptxt << endl;
    out << endl;
}

void CmdList::runCommands(std::vector<string>::iterator &arg_it,
                          const std::vector<string>::iterator &end_it)
{
    while(arg_it != end_it) {
        string arg_cmd = *arg_it;
        if(arg_cmd[0] != '-') {
            cerr << arg_cmd << endl;
            cerr << "All commands must begin with '-'" << endl;
            exit(1);
        }
        arg_cmd = arg_cmd.substr(1);
        arg_it++;
        
        bool found = true;
        for(auto &cmd : commands) {
            if(arg_cmd == cmd.name) {
                cmd.body(arg_it, end_it);
                found = true;
                break;
            }
        }
        if(!found) {
            cerr << "Command -" + arg_cmd + " is not recognized" << endl;
            exit(1);
        }
    }
}


std::function< void(
    std::vector<string>::iterator &,
    const std::vector<string>::iterator &
) >
genericBinaryOp(
    std::function< void(CorkTriMesh in0, CorkTriMesh in1, CorkTriMesh *out) > binop,
    bool needsplit
) {
    return [binop, needsplit]
    (std::vector<string>::iterator &args,
     const std::vector<string>::iterator &end) {
        // data...
        std::vector<CorkTriMesh> in;
        CorkTriMesh out;
        
        if(args == end) { cerr << "too few args" << endl; exit(1); }
        loadMesh(*args, in, needsplit);
        args++;
        
        if(args == end && in.size() < 2) { cerr << "too few args" << endl; exit(1); }
        loadMesh(*args, in, needsplit);
        args++;
        if(in.size() == 2){
            binop(in.front(), in.back(), &out);
        }
        else {
            CorkTriMesh *out = nullptr;
            for(auto inval = in.begin()+1; inval != in.end(); ++inval) {
                bool first = out == nullptr;
                CorkTriMesh *tmp = !first ? out : &in.front();
                out = new CorkTriMesh();
                std::cerr << tmp->n_vertices << "/" << tmp->n_triangles << "  " << inval->n_vertices << "/" << inval->n_triangles << std::endl;
                binop(*tmp, *inval, out);
                if(!first) {
                    freeCorkTriMesh(tmp);
                    delete tmp;
                }
            }
            if(out){
                freeCorkTriMesh(out);
                delete out;
            }
        }
        
        if(args == end) { cerr << "too few args" << endl; exit(1); }
        saveMesh(*args, out);
        args++;
        
        freeCorkTriMesh(&out);
        
        for(auto &inv: in)
            freeCorkTriMesh(&inv);
    };
}


int main(int argc, char *argv[])
{
    initRand(); // that's useful
    
    if(argc < 2) {
        cout << "Please type 'cork -help' for instructions" << endl;
        exit(0);
    }

    bool needsplit = false;
    bool needsave = false;
    // store arguments in a standard container
    std::vector<string> args;
    for(int k=0; k<argc; k++) {
        if(std::strcmp(argv[k], "-split") == 0)
            needsplit = true;
        if(std::strcmp(argv[k], "-save") == 0)
            needsave = true;
        else
            args.push_back(argv[k]);
    }
    
    auto arg_it = args.begin();
    // *arg_it is the program name to begin with, so advance!
    arg_it++;
    CmdList cmds;
    
    // add cmds
    cmds.regCmd("solid",
    "-solid in                           Determine whether the input mesh represents\n"
    "                                    a solid object.  (aka. watertight) (technically\n"
    "                                    solid == closed and non-self-intersecting)",
    [&needsplit, &needsave](std::vector<string>::iterator &args,
       const std::vector<string>::iterator &end) {
        std::vector<CorkTriMesh> in;
        if(args == end) { cerr << "too few args" << endl; exit(1); }
        string filename = *args;
        loadMesh(*args, in, needsplit);
        args++;
        int cnt = 0;
        for(auto &i: in) {
            bool solid = isSolid(i);
            cout << "The mesh #" << cnt << " " << filename << " is: " << endl;
            cout << "    " << ((solid)? "SOLID" : "NOT SOLID") << endl;
            ++cnt;
        }
        cnt = 0;
        for(auto &i: in) {
            if(needsave) {
                saveMesh(filename.substr(0,filename.length()-4)+"_part"+std::to_string(cnt)+filename.substr(filename.length()-4), i);
            }
            freeCorkTriMesh(&i);
            ++cnt;
        }
    });
    cmds.regCmd("union",
    "-union in0 ... inN [-split] out     Compute the Boolean union of in0 and in1,\n"
    "                                    and output the result\n"
    "                                    split - optional for 2 and more obj in file",
    genericBinaryOp(computeUnion, needsplit));
    cmds.regCmd("diff",
    "-diff in0 ... inN [-split] out      Compute the Boolean difference of in0 and in1,\n"
    "                                    and output the result\n"
    "                                    split - optional for 2 and more obj in file",
    genericBinaryOp(computeDifference, needsplit));
    cmds.regCmd("isct",
    "-isct in0 ... inN [-split] out      Compute the Boolean intersection of in0 and in1,\n"
    "                                    and output the result\n"
    "                                    split - optional for 2 and more obj in file",
    genericBinaryOp(computeIntersection, needsplit));
    cmds.regCmd("xor",
    "-xor in0 ... inN [-split] out       Compute the Boolean XOR of in0 and in1,\n"
    "                                    and output the result\n"
    "                                    (aka. the symmetric difference)\n"
    "                                    split - optional for 2 and more obj in file",
    genericBinaryOp(computeSymmetricDifference, needsplit));
    cmds.regCmd("resolve",
    "-resolve in0 ... inN [-split] out   Intersect the two meshes in0 and in1,\n"
    "                                    and output the connected mesh with those\n"
    "                                    intersections made explicit and connected\n"
    "                                    split - optional for 2 and more obj in file",
    genericBinaryOp(resolveIntersections, needsplit));
    
    
    cmds.runCommands(arg_it, args.end());
    
    return 0;
}









