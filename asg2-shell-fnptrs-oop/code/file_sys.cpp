// $Id: file_sys.cpp,v 1.5 2016-01-14 16:16:52-08 - - $

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <sstream>

using namespace std;

#include "debug.h"
#include "file_sys.h"

int inode::next_inode_nr {1};

struct file_type_hash {
   size_t operator() (file_type type) const {
      return static_cast<size_t> (type);
   }
};

ostream& operator<< (ostream& out, file_type type) {
   static unordered_map<file_type,string,file_type_hash> hash {
      {file_type::PLAIN_TYPE, "PLAIN_TYPE"},
      {file_type::DIRECTORY_TYPE, "DIRECTORY_TYPE"},
   };
   return out << hash[type];
}

inode::inode(file_type type): inode_nr (next_inode_nr++) {
   switch (type) {
      case file_type::PLAIN_TYPE:
           contents = make_shared<plain_file>();
           break;
      case file_type::DIRECTORY_TYPE:
           contents = make_shared<directory>();
           break;
   }
   DEBUGF ('i', "inode " << inode_nr << ", type = " << type);
}

plain_file_ptr plain_file_ptr_of (base_file_ptr ptr) {
   plain_file_ptr pfptr = dynamic_pointer_cast<plain_file> (ptr);
   if (pfptr == nullptr) throw invalid_argument ("plain_file_ptr_of");
   return pfptr;
}

directory_ptr directory_ptr_of (base_file_ptr ptr) {
  directory_ptr dirptr = dynamic_pointer_cast<directory> (ptr);
  if (dirptr == nullptr) {
    throw invalid_argument ("directory_ptr_of");
  }
  return dirptr;
}

file_type inode::get_type(){
   file_type thetype = this->type;
   return thetype;
}

// inode_state -
//    A small convenient class to maintain the state of the simulated
//    process:  the root (/), the current directory (.), and the
//    prompt.

inode_state::inode_state() {
   inode initial_dir(DIRECTORY_TYPE);
   inode_ptr the_root = make_shared<inode>(initial_dir);
   cout << "root type: " << the_root->type << endl;
   directory_ptr curr_dir = directory_ptr_of(initial_dir.contents);
   (*curr_dir).sized = 2;
   curr_dir->dirents.insert({".", the_root});
   curr_dir->dirents.insert({"..", the_root});
   cwd = the_root;
   root = the_root;
   pwd.push_back("/");
   DEBUGF ('i', "root = " << root << ", cwd = " << cwd
          << ", prompt = \"" << prompt() << "\"");
}

const string& inode_state::prompt() { return prompt_; }


// helper function to get to directory at end of path
// to then do operations
void directory::get_to_dir(inode_state& state, const wordvec& words){
   state.tmp_cwd = state.cwd;
   map<string, inode_ptr>::iterator it;
   // grab path and split it by slash into new wordvec
   string path = words.at(1);
   string dir;
   stringstream to_split(path);
   vector<string> dirs;
   while(getline(to_split, dir, '/')){
      dirs.push_back(dir);
   }
   for(unsigned int i = 0; i < dirs.size(); ++i){
      directory_ptr curr_dir = directory_ptr_of((*state.tmp_cwd).contents);
      it = (*curr_dir).dirents.find(dirs.at(i));
      if(it != (*curr_dir).dirents.end()){
         state.tmp_cwd = it->second;
         state.tmp_pwd.push_back(it->first);
      }else{
          cout<< dirs.at(i)<< ": Invalid directory.\n";
          break;
       }
   } 
}

void directory::get_to_file(inode_state& state, const wordvec& words){
   state.tmp_cwd = state.cwd;
   map<string, inode_ptr>::iterator it;
   string path = words.at(1);
   string dir;
   stringstream to_split(path);
   vector<string> dirs;
   while(getline(to_split, dir, '/')){
      dirs.push_back(dir);
   }
   for(unsigned int i = 0; i < dirs.size() - 1; ++i){
      directory_ptr curr_dir = directory_ptr_of((*state.tmp_cwd).contents);
      it = (*curr_dir).dirents.find(dirs.at(i));
      if(it != (*curr_dir).dirents.end()){
         state.tmp_cwd = it->second;
         state.tmp_pwd.push_back(it->first);
      }else{
          cerr<< dirs.at(i)<< ": Invalid path.\n";
          break;
      }
   }
}
// class inode -
// inode ctor -
//    Create a new inode of the given type.
// get_inode_nr -
//    Retrieves the serial number of the inode.  Inode numbers are
//    allocated in sequence by small integer.
// size -
//    Returns the size of an inode.  For a directory, this is the
//    number of dirents.  For a text file, the number of characters
//    when printed (the sum of the lengths of each word, plus the
//    number of words.
//  

int inode::get_inode_nr() const {
   DEBUGF ('i', "inode = " << inode_nr);
   return inode_nr;
}


file_error::file_error (const string& what):
            runtime_error (what) {
}

// class plain_file -
// Used to hold data.
// synthesized default ctor -
//    Default vector<string> is a an empty vector.
// readfile -
//    Returns a copy of the contents of the wordvec in the file.
// writefile -
//    Replaces the contents of a file with new contents.

size_t plain_file::size() const {
  size_t size = 0;
  for(auto i = data.begin(); i != data.end(); i++){
    size += i->size();
    size++;
  }
  if (size > 1){
   return (size-1);
  }else{
    return 0;
  }
}

const wordvec& plain_file::readfile() const {
   DEBUGF ('i', data);
   return data;
}

void plain_file::writefile (const wordvec& words) {
  for(size_t i = 2; i < words.size(); i++){
      this->data.push_back(words.at(i));
   }
   DEBUGF ('i', words);
}

void plain_file::remove (const string&) {
   throw file_error ("is a plain file");
}


//inode_ptr plain_file::mkfile (const string&) {
//   throw file_error ("is a plain file");
//}

// class directory -
// Used to map filenames onto inode pointers.
// default ctor -
//    Creates a new map with keys "." and "..".
// remove -
//    Removes the file or subdirectory from the current inode.
//    Throws an file_error if this is not a directory, the file
//    does not exist, or the subdirectory is not empty.
//    Here empty means the only entries are dot (.) and dotdot (..).
// mkdir -
//    Creates a new directory under the current directory and 
//    immediately adds the directories dot (.) and dotdot (..) to it.
//    Note that the parent (..) of / is / itself.  It is an error
//    if the entry already exists.
// mkfile -
//    Create a new empty text file with the given name.  Error if
//    a dirent with that name exists.

size_t directory::size() const {
   size_t size {0};
   DEBUGF ('i', "size = " << size);
   return size;
}

const wordvec& directory::readfile() const {
   throw file_error ("is a directory");
}

void directory::writefile (const wordvec&) {
   throw file_error ("is a directory");
}

void directory::remove (const string& filename) {
   DEBUGF ('i', filename);
}

directory_ptr inode_state::get_cwd(){
   directory_ptr curr_dir = directory_ptr_of(this->cwd->contents);
   return curr_dir;
}

void directory::print_path(inode_state& state){
   for(unsigned int i = 0; i < state.pwd.size(); ++i){
      if(i == 0 and state.pwd.size() == 1){
         cout<< "/:\n";
         break;
      }
      if(i == 0){
         cout<< "/";
      }else{
          if(i == state.pwd.size() - 1){
             cout<< state.pwd.at(i)<< ":\n";
          }else{
              cout<< state.pwd.at(i)<< "/";
           }
       }
   }     
}

void directory::print_tmp_path(inode_state& state){
   for(unsigned int i = 0; i < state.tmp_pwd.size(); ++i){
      if(i == 0 and state.tmp_pwd.size() == 1){
         cout<< "/:\n";
         break;
      }
      if(i == 0){
         cout<< "/";
      }else{
          if(i == state.tmp_pwd.size() - 1){
             cout<< state.tmp_pwd.at(i)<< ":\n";
          }else{
              cout<< state.tmp_pwd.at(i)<< "/";
           }
       }
   }   
}

inode_ptr directory::mkfile(inode_state& state, const wordvec& words){
   map<string, inode_ptr>::iterator iter;
   inode file(PLAIN_TYPE);
   inode_ptr name = make_shared<inode>(file);
   plain_file_ptr curr_file = plain_file_ptr_of(name->contents);
   (*curr_file).writefile(words);
   (*curr_file).sizep = (*curr_file).size();
   //(*curr_file).sizep = words.size() - 2; // this returns the number of words after the cmd
   //cout<< "      "<< (*curr_file).size() << "\n";
   get_to_file(state, words);
   
   string path = words.at(1);
   string dir;
   stringstream to_split(path);
   vector<string> dirs;
   while(getline(to_split, dir, '/')){
      dirs.push_back(dir);
   }
   directory_ptr curr_dir = directory_ptr_of(state.tmp_cwd->contents);
   iter = (*curr_dir).dirents.find(dirs.at(dirs.size() - 1));
   if(iter != (*curr_dir).dirents.end()){
      inode_ptr tmp_check = iter->second;
      if((*tmp_check).type == DIRECTORY_TYPE){
         cerr<<"Error: mkdir: "<< dirs.at(dirs.size() - 1)<<
               " is a directory!\n";
      }
      if((*tmp_check).type == PLAIN_TYPE){
         plain_file_ptr to_replace = plain_file_ptr_of((*tmp_check).contents);
         (*to_replace).data = (*curr_file).data;
         (*to_replace).sizep = (*curr_file).sizep;
      }
   }
   (*curr_dir).dirents.insert({dirs.at(dirs.size() - 1), name});
   ++this->sized;
   return name;
}

inode_ptr directory::mkdir (inode_state& state, const wordvec& words){
  inode_ptr the_dir = make_shared<inode>(file_type::DIRECTORY_TYPE);
  cout << "root type: " << the_dir->type << endl;
   /*inode new_dir(DIRECTORY_TYPE);
   inode_ptr the_dir = make_shared<inode>(new_dir);
   string path = words.at(1);
   string dir;
   stringstream to_split(path);
   vector<string> dirs;
   while(getline(to_split, dir, '/')){
      dirs.push_back(dir);
   }
   get_to_file(state, words);

   directory_ptr direc_ptr = directory_ptr_of(state.tmp_cwd->contents);
   ++(*direc_ptr).sized;   
   (*direc_ptr).dirents.insert({dirs.at(dirs.size() - 1), the_dir});
   inode_ptr tmp = state.tmp_cwd;
   state.tmp_cwd = the_dir;
   directory_ptr the_new_dir = directory_ptr_of(state.tmp_cwd->contents);
   (*the_new_dir).dirents.insert({".", state.tmp_cwd});
   (*the_new_dir).dirents.insert({"..", tmp});
   (*the_new_dir).sized = 2;
   */
   return the_dir;
}

void directory::ls(inode_state& state, const wordvec& words){
   inode_ptr tmp;
   int nr;
   //file_type type;
   size_t the_size;
   if(words.size() == 1){
      print_path(state);
      directory_ptr curr_dir = directory_ptr_of((*state.cwd).contents);
      for(map<string, inode_ptr>::iterator iter = (*curr_dir).dirents.begin(); iter != (*curr_dir).dirents.end(); ++iter){
      	//cout<< "hello\n";
      	 tmp = iter->second;
         nr = (*tmp).inode_nr;
         the_size = (*curr_dir).dirents.size();
         //type = (*tmp).get_type();
         //cout<< type << "\n";
         //plain_file_ptr plain_ptr = plain_file_ptr_of((*tmp).contents);

         if(iter->first.compare(".") != 0 && iter->first.compare("..") != 0){
            plain_file_ptr plain_ptr = plain_file_ptr_of((*tmp).contents);
            the_size = (*plain_ptr).sizep;
         }

         cout<< "      "<< nr;
         cout << "      "<< the_size;
         cout<< "  "<< iter->first<< "\n";        
         /*
         tmp = iter->second;
         nr = (*tmp).inode_nr;
         //cout<< "      "<< nr;
         type = (*tmp).get_type();
         if(type == DIRECTORY_TYPE){
            directory_ptr dir_ptr = directory_ptr_of((*tmp).contents);
            the_size = (*dir_ptr).sized;
            cout<< "      "<< the_size;
            if(iter->first.compare(".") == 0 or iter->first.compare("..") == 0){
               cout<< "  "<< iter->first<< "\n";
            }else{
                cout<< "  "<< iter->first<< "/\n";
             }
         }
         if(type == PLAIN_TYPE){
            plain_file_ptr plain_ptr = plain_file_ptr_of((*tmp).contents);
            the_size = (*plain_ptr).sizep;
            cout<< "      "<< the_size;
            cout<< "  "<< iter->first<< "\n";
         }*/
      }      
      
   }
   
}

ostream& operator<< (ostream& out, const inode_state& state) {
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

