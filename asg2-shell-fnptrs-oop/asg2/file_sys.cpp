// $Id: file_sys.cpp,v 1.5 2016-01-14 16:16:52-08 - - $

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <iomanip>

using namespace std;

#include "debug.h"
#include "file_sys.h"
#include "commands.h"


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

//const string& inode_state::prompt() { return prompt_; }

ostream& operator<< (ostream& out, const inode_state& state) {
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

inode::inode(file_type init_type): inode_nr (next_inode_nr++), type (init_type) {
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

int inode::get_inode_nr() const {
   DEBUGF ('i', "inode = " << inode_nr);
   return inode_nr;
}

plain_file_ptr plain_file_ptr_of (base_file_ptr ptr) {
   plain_file_ptr pfptr = dynamic_pointer_cast<plain_file> (ptr);
   if (pfptr == nullptr) throw invalid_argument ("plain_file_ptr_of");
   return pfptr;
}

directory_ptr directory_ptr_of (base_file_ptr ptr) {
   directory_ptr dirptr = dynamic_pointer_cast<directory> (ptr);
   if (dirptr == nullptr) throw invalid_argument ("directory_ptr_of");
   return dirptr;
}


file_error::file_error (const string& what):
            runtime_error (what) {
}

size_t plain_file::size() const {
   size_t size {0};
   DEBUGF ('i', "size = " << size);
   return size;
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

inode_ptr plain_file::mkdir (const string&) {
   throw file_error ("is a plain file");
}

inode_ptr plain_file::mkfile (const string&) {
   throw file_error ("is a plain file");
}


size_t directory::size() const {
   size_t size {2};
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

void directory::df_clear() {
    for (auto it : this->dirents) {
        if (it.second->getType() == DIRECTORY_TYPE
         //node->getType()
                && it.first != "."
                && it.first != "..") {
            directory_ptr_of(it.second->contents)->df_clear();
        }
    }
    this->dirents.clear();
}

inode_state::~inode_state() {
    directory_ptr_of(root->contents)->df_clear();
}

inode_state::inode_state() {
   root = make_shared<inode>(DIRECTORY_TYPE);
   //cout << "root type: " << root->type << endl;
   cwd = root;
   directory_ptr_of(root->contents)->setRoot(root);
   prompt_ = "%";
   /*DEBUGF ('i', "root = " << root << ", cwd = " << cwd
          << ", prompt = \"" << prompt() << "\"");*/
}

string inode_state::getPrompt() const {
   return prompt_;
}

void inode_state::setPrompt(string newPrompt) {
   //changes prompt display
   prompt_ = newPrompt;
}



int inode::getType() const {
   return type;
}

size_t inode::size() const {
  if(type == DIRECTORY_TYPE){
    return directory_ptr_of(contents)->size();
  }
  return plain_file_ptr_of(contents)->size();
}

inode_ptr inode_state::findPath(const string& path){
   inode_ptr curr;
   directory_ptr dir;
   string pathCopy = path;

   if(path.at(0) == '/') {
      pathCopy = path.substr(1);
      curr = root;
   } else {
      curr = cwd;
   }
   wordvec p = split(pathCopy, "/");
   
   for(string word : p) {
      if(p.at(p.size()-1) == word && path.back() != '/') {
         break;
      } 
      dir = directory_ptr_of(curr->getContents());
      curr = dir->getChild(word);
      if(curr == nullptr) {
         return curr;
      }      
   }
   return curr;
}

inode_ptr directory::getChild(const string& name) {
   auto entry = dirents.find(name);
   if(entry == dirents.end()){
      return nullptr;
   }
   return entry->second;
}

string inode::getName() const {
   return name;
}

base_file_ptr inode::getContents() const {
   return contents;
}

void inode_state::pwd(const wordvec& words) {
   if(words.size() > 1){
      throw command_error ("Too many arguments to pwd");
   }
   directory_ptr dir = directory_ptr_of(cwd->getContents());
   cout << getCwd() << endl;
}

void inode_state::ls(const wordvec& words) {
   if(words.size() == 1) {
      directory_ptr dir = directory_ptr_of(cwd->getContents());
      cout << getCwd() << ":" << endl;
      dir->ls();
   } else {
      for(unsigned int i = 1; i < words.size(); i++){
         string path = words.at(i);
         inode_ptr node = findPath(path);
         if(node == nullptr){
            throw invalid_argument ("path given to ls not found");
         }  else if(node->getType() == PLAIN_TYPE) {
            throw invalid_argument ("ls on plain inode not implemented");
         }
         directory_ptr dir = directory_ptr_of(node->getContents());
         cout << path << ":" << endl;
         dir->ls();
      }
   }
   
}

void directory::ls() {
   for(auto i = dirents.begin(); i != dirents.end(); i++){
      string slash = "";
      if(i->second->getType() == DIRECTORY_TYPE && 
            i->first != "." && 
            i->first != "..") {
         slash = "/";
      } else {
         slash = "";
      }
      (cout << setw(6) << i->second->get_inode_nr() << setw(6) <<
            i->second->size() << "\t" << i->first << slash << endl);
   }
}

string inode_state::getCwd() const {
   inode_ptr curr = cwd;
   directory_ptr currContents;
   wordvec path;
   string name = "/";
   while(curr != root) {
      path.push_back(curr->getName());
      currContents = directory_ptr_of(curr->contents);
      curr = currContents->getChild("..");
   }
   while(path.size() > 0) {
      name.append(path.back());
      name.append("/");
      path.pop_back();
   }
   return name;
}

void inode::setName(const string& newName) {
   name = newName;
}

void directory::setRoot(inode_ptr root) {
   dirents.insert(make_pair(".", root));
   dirents.insert(make_pair("..", root));
   root->setName("/");
}

void directory::setParentChild(inode_ptr parent, inode_ptr child) { 
   dirents.insert(make_pair(".", child));
   dirents.insert(make_pair("..", parent));
}

void inode_state::cdRoot() {
   cwd = root;
}

void inode_state::cd(const string& path) {
   string newPath = path;
   if(newPath.back() != '/') {
      newPath.append("/");
   }
   inode_ptr setDir = findPath(newPath);
   if(setDir == nullptr) {
      throw command_error ("Directory not found");
   } else {
      cwd = setDir;
   }
}

void inode_state::mkdir(const string& path) {
   string name = path;
   if (path.back() == '/') { //trim ending '/' for findpath
      //cout<<name << "\n";
      name = path.substr(0, path.size() - 1);
      //cout<< "hello";
      //cout<<name << "\n";
   }
   //cout<< path.back() << "\n";
   inode_ptr p = findPath(name);
   if(p == nullptr) {
      throw command_error ("Invalid path");
   }
   directory_ptr dir = directory_ptr_of(p->getContents());
   size_t index = name.find_last_of("/");
   if (index == string::npos){  //if no '/' is found
      dir->mkdir(name);
   } else {
      dir->mkdir(name.substr(index+1));
   }
}
inode_ptr directory::mkdir(const string& name) {
   if(dirents.find(name) != dirents.end()) {
      throw command_error ("trying to create existing directory");
   }
   inode_ptr parent = dirents.at(".");
   inode_ptr node = make_shared<inode>(DIRECTORY_TYPE);
   node->setName(name);
   directory_ptr dir = directory_ptr_of(node->getContents());
   dir->setParentChild(parent, node); 
   dirents.insert(make_pair(name, node));
   return node;
}

void inode_state::mkfile(const string& filename) {
   if(filename.size() == 0) throw command_error ("you need to have a file name");
   cout<< "this is not working yet" << "\n";
   //map<string, inode_ptr>::iterator iter;
   //inode_ptr name = make_shared<inode>(PLAIN_TYPE);
   //cout << "root type: " << name->type << endl;
   //name->setName(filename);
   //plain_file_ptr curr_file = plain_file_ptr_of(name->contents);
   //string fname = filename;
   //cout<< "hello";
}

inode_ptr directory::mkfile (const string& filename) {
   //cout<< "hello";
   DEBUGF ('i', filename);
   return nullptr;
}

