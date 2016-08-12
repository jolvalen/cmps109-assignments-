// $Id: main.cpp,v 1.8 2015-04-28 19:23:13-07 - - $

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include <regex>

using namespace std;

#include "listmap.h"
#include "xpair.h"
#include "util.h"

using str_str_map = listmap<string,string>;
using str_str_pair = str_str_map::value_type;

void scan_options (int argc, char** argv) {
   opterr = 0;
   for (;;) {
      int option = getopt (argc, argv, "@:");
      if (option == EOF) break;
      switch (option) {
         case '@':
            traceflags::setflags (optarg);
            break;
         default:
            complain() << "-" << char (optopt) << ": invalid option"
                       << endl;
            break;
      }
   }
}

const string cin_name = "-";

void process (istream& infile, const string& filename) {
   // this part deals with the comments and the blank lines
   regex comment_regex {R"(^\s*(#.*)?$)"};
   // check that there are no "=" signs
   regex trimmed_regex {R"(^\s*([^=]+?)\s*$)"};
   // this part deals with the signs
   regex key_value_regex {R"(^\s*(.*?)\s*=\s*(.*?)\s*$)"};
   str_str_map test;
   //cout << filename << ":" << endl;
   
   //static string colons (32, ':');
   //cout << colons << endl << filename << endl << colons << endl;
   int linecount = 1;
   for(;;) {
      string line;
      getline (infile, line);
      if (infile.eof()) break;
      smatch result;

      if (regex_search (line, result, comment_regex)) {
         cout << filename << ": "<< linecount << ": "<< line << endl;
      }

      // this part deals with the lines that have no equal sign;
      else if (regex_search (line, result, trimmed_regex)) {
         cout << filename << ": "<< linecount << ": "<< line << endl;

         //iterator helper to get the results of finding the keys
         str_str_map::iterator itorG = test.find(result[1]);
         // this is default to key not found
         string found = "key not found";
         string found1 = ": ";
         // if itorG does not point to test.end()
         if (itorG != test.end()){
            found = (*itorG).second; // then get the found contents;
            found1 = " = ";
         }

         cout << line << found1 << found << endl;
      }

      else if (regex_search (line, result, key_value_regex)) {
         // this rst1 string helps assign key to value
         string rst1 = result[1];
         // this rst2 string represents value
         string rst2 = result[2];

         // this if- handles the "=", so it prints 
         // all the insertions to test 
         if ((rst1.size() == 0) && (rst2.size() == 0)){
            cout << filename << ": "<< linecount << ": "<< line << endl;
            for (str_str_map::iterator itor = test.begin();
               itor != test.end(); ++itor) {
               cout << (*itor).first << " = " << (*itor).second << endl;
            }
         }

         // this if- handles the "key = value", 
         // so it inserts items to the test
         if ((rst1.size() != 0) && (rst2.size() != 0)){
            // this parts matches rst1 and rst2 
            str_str_pair newPair (rst1, rst2);
            // this adds it to the test (to the list).
            test.insert(newPair); 
            cout << filename << ": "<< linecount << ": "<< line << endl;
            cout << rst1 << " = " << rst2 << endl;
         }

         // this if- handles the " =value", 
         // so it has to print items from the test
         if ((rst1.size() == 0) && (rst2.size() != 0)){
            cout << filename << ": "<< linecount << ": "<< line << endl;
            for (str_str_map::iterator itor = test.begin();
               itor != test.end(); ++itor) {
               if ((*itor).second == result[2]){
                  cout << (*itor).first << " = " 
                     << (*itor).second << endl;
               }
            }
         }

         // this if- handles the " key=", 
         // so it has to delete the key from the test
         if ((rst1.size() != 0) && (rst2.size() == 0)){
            cout << filename << ": "<< linecount << ": "<< line << endl;
            str_str_map::iterator itorR = test.find(result[1]);
            test.erase (itorR);
         } 
      }
      linecount++;
   }
   
}

int main (int argc, char** argv) {
   
   int status = 0;
   sys_info::set_execname (argv[0]);
   //scan_options (argc, argv);

   string progname = sys_info::get_execname();
   //string progname ((argv[0]));
   
   str_str_map filenames;
   //vector<string> filenames (&argv[1], &argv[argc]);

   // this loop adds the files into a str_str_map filenames
   for (char** argp = &argv[optind]; argp != &argv[argc]; ++argp) {
      str_str_pair pair (*argp, to_string<int> (argp - argv));
      filenames.insert (pair);
   }
   
   //if (filenames.size() == 0) filenames.push_back (cin_name);
   str_str_pair default_ ("-", " ");
   if(filenames.empty()) filenames.insert(default_);

   for (str_str_map::iterator itor = filenames.begin();
        itor != filenames.end(); ++itor) {
        
         string filename = (*itor).first;
         //cout << filename << endl; 
         if (filename == cin_name) process (cin, filename);
         else {
            ifstream infile (filename);
            if (infile.fail()) {
               status = 1;
               cerr << progname << ": " << filename << ": " 
               "No such file or directory" << endl;
            }else {
               process (infile, filename);
               infile.close();
            }
         }
   }

   //filenames.erase(itor);
   return status;
}

