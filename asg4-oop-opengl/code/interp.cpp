// $Id: interp.cpp,v 1.2 2015-07-16 16:57:30-07 - - $

#include <memory>
#include <string>
#include <vector>
using namespace std;

#include <GL/freeglut.h>

#include "debug.h"
#include "interp.h"
#include "shape.h"
#include "util.h"

static unordered_map<string,void*> fontcode {
   {"Fixed-8x13"    , GLUT_BITMAP_8_BY_13       },
   {"Fixed-9x15"    , GLUT_BITMAP_9_BY_15       },
   {"Helvetica-10"  , GLUT_BITMAP_HELVETICA_10  },
   {"Helvetica-12"  , GLUT_BITMAP_HELVETICA_12  },
   {"Helvetica-18"  , GLUT_BITMAP_HELVETICA_18  },
   {"Times-Roman-10", GLUT_BITMAP_TIMES_ROMAN_10},
   {"Times-Roman-24", GLUT_BITMAP_TIMES_ROMAN_24},
};

unordered_map<string,interpreter::interpreterfn>
interpreter::interp_map {
   {"define" , &interpreter::do_define },
   {"draw"   , &interpreter::do_draw   },
   {"moveby" , &interpreter::do_moveby},
   {"border" , &interpreter::do_border},
};

unordered_map<string,interpreter::factoryfn>
interpreter::factory_map {
   {"text"     , &interpreter::make_text     },
   {"ellipse"  , &interpreter::make_ellipse  },
   {"circle"   , &interpreter::make_circle   },
   {"polygon"  , &interpreter::make_polygon  }, 
   {"rectangle", &interpreter::make_rectangle}, //updated
   {"square"   , &interpreter::make_square   }, //updated
   {"triangle" , &interpreter::make_triangle }, //new
   {"equilateral" , &interpreter::make_equilateral },//new
   {"diamond"  , &interpreter::make_diamond  }, //new
};

interpreter::shape_map interpreter::objmap;

interpreter::~interpreter() {
   for (const auto& itor: objmap) {
      cout << "objmap[" << itor.first << "] = "
           << *itor.second << endl;
   }
}

void interpreter::interpret (const parameters& params) {
   DEBUGF ('i', params);
   param begin = params.cbegin();
   string command = *begin;
   auto itor = interp_map.find (command);
   if (itor == interp_map.end()) throw runtime_error ("syntax error");
   interpreterfn func = itor->second;
   func (++begin, params.cend());
}

void interpreter::do_define (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   string name = *begin;
   objmap.emplace (name, make_shape (++begin, end));
}


void interpreter::do_draw (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   if (end - begin != 4) throw runtime_error ("syntax error");
   string name = begin[1];
   shape_map::const_iterator itor = objmap.find (name);
   if (itor == objmap.end()) {
      throw runtime_error (name + ": no such shape");
   }
   rgbcolor color {begin[0]};
   vertex where {from_string<GLfloat> (begin[2]),
                 from_string<GLfloat> (begin[3])};
   //itor->second->draw (where, color);
   window::push_back(object(itor->second, where, color));
}

void interpreter::do_moveby (param begin, param end) {
    DEBUGF ('f', range (begin, end));
    window::moveby = atoi(begin[0].c_str());
}

void interpreter::do_border (param begin, param end) {
    DEBUGF ('f', range (begin, end));
    //window::border_color = rgbcolor(begin[0]);
    rgbcolor color {begin[0]};
    window::border_color = color;
    window::selected_border_thickness = atoi(begin[1].c_str());
}

shape_ptr interpreter::make_shape (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   string type = *begin++;
   auto itor = factory_map.find(type);
   if (itor == factory_map.end()) {
      throw runtime_error (type + ": no such shape");
   }
   factoryfn func = itor->second;
   return func (begin, end);
}

shape_ptr interpreter::make_text (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   auto fontpair = fontcode.find(*begin);
   if (fontpair == fontcode.end()) {
      throw runtime_error (*begin + ": invalid font");
   }

   ostringstream textstream;
   begin++;
   while (begin != end) {
      textstream << *begin;
      begin++;
      if (begin != end) textstream << " ";
   }
   const string consttext = textstream.str();

   return make_shared<text> (fontpair->second, consttext);
}

shape_ptr interpreter::make_ellipse (param begin, param end) {
  DEBUGF ('f', range (begin, end));
  auto iter = begin;
  GLfloat width = stof(*iter++);
  GLfloat height = stof(*iter);
  return make_shared<ellipse> (width, height);
}

shape_ptr interpreter::make_circle (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   if (begin == end)
      throw runtime_error("Circle defined without diameter parameter.");
   if (end - begin != 1) 
    throw runtime_error ("circle: only diameter parameter needed");
   GLfloat diameter = stof(begin[0]);
   return make_shared<circle>(diameter);
}

shape_ptr interpreter::make_polygon (param begin, param end) {
   DEBUGF ('f', range (begin, end));
    vertex_list vertices;
    for (auto iter = begin; iter != end;) {
        GLfloat xPos = stof(*iter++);
        GLfloat yPos = stof(*iter++);
        vertex vex {xPos, yPos}; 
        vertices.push_back(vex);
    }
    return make_shared<polygon> (vertices);
}

/*
//updated
shape_ptr interpreter::make_rectangle (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   GLfloat width = stof(begin[0]);
   GLfloat height = stof(begin[1]);
   return make_shared<rectangle> (width, height);
}

*/

//updated
shape_ptr interpreter::make_rectangle (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   auto iter = begin;
   GLfloat width = stof(*iter++);
   GLfloat height = stof(*iter);
   return make_shared<rectangle> (width, height);
}

//updated
shape_ptr interpreter::make_square (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   if (begin == end)
      throw 
        runtime_error("Square defined without side lenght parameter.");
   if (end - begin != 1) 
    throw runtime_error ("square: only side lenght needed");
   auto iter = begin;
   GLfloat side = stof(*iter);
   return make_shared<square> (side);
}

//new
shape_ptr interpreter::make_triangle (param begin, param end) {
    DEBUGF ('f', range (begin, end));
    vertex_list vertices;
    for (auto iter = begin; iter != end;) {
        GLfloat xPos = stof(*iter++);
        GLfloat yPos = stof(*iter++);
        vertex vex {xPos, yPos}; 
        vertices.push_back(vex);
    }
    return make_shared<triangle> (vertices);
}

//new
shape_ptr interpreter::make_equilateral (param begin, param end) {
   DEBUGF ('f', range (begin, end));
   GLfloat width = stof(begin[0]);
   return make_shared<equilateral> (width);
}

//new
shape_ptr interpreter::make_diamond (param begin, param end) {
    DEBUGF ('f', range (begin, end));
    GLfloat width = stof(begin[0]);
    GLfloat height = stof(begin[1]);
    return make_shared<diamond> (width, height);
}

