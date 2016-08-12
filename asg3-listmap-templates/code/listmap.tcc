// $Id: listmap.tcc,v 1.8 2016-05-04 13:49:56-07 - - $

#include "listmap.h"
#include "trace.h"

//
/////////////////////////////////////////////////////////////////
// Operations on listmap::node.
/////////////////////////////////////////////////////////////////
//

//
// listmap::node::node (link*, link*, const value_type&)
//
template <typename Key, typename Value, class Less>
listmap<Key,Value,Less>::node::node (node* next, node* prev,
                                     const value_type& value):
            link (next, prev), value (value) {
}

//
/////////////////////////////////////////////////////////////////
// Operations on listmap.
/////////////////////////////////////////////////////////////////
//

//
// listmap::~listmap()
//
template <typename Key, typename Value, class Less>
listmap<Key,Value,Less>::~listmap() {
   //TRACE ('l', (void*) this);
   while (!empty()){
      erase(begin());
   }
}


//
// iterator listmap::insert (const value_type&)
//
template <typename Key, typename Value, class Less>
typename listmap<Key,Value,Less>::iterator
listmap<Key,Value,Less>::insert (const value_type& pair) {
   TRACE ('l', &pair << "->" << pair);

   // first check if the list is empty, 
   if (empty()){
      node* curr = new node(anchor(), anchor(), pair);
      // set the link anchor_ pointers to the first node
      anchor_.next = curr; // next item on the list is curr
      anchor_.prev = curr; // previous item on the list is also curr
     return iterator(curr);
   } 

   // if it's not empty, then add to the existing list
   iterator itor; // pointer itor to iterate through the list
   for(itor = begin(); itor != end(); ++itor){
      // check if the key exists, if it does then overwrite it.
      if (pair.first == itor->first){
         itor->second = pair.second;
         return itor;
      }
         // this part will handle the increasing lexico- graphic order part
      	// so if [itor > curr] then we are going to insert the item before
      	// the itor. 
         if(less(pair.first, itor->first)){ 
         node* curr = new node(itor.where, itor.where->prev, pair);
         itor.where->prev->next = curr; // re-assign pointers
         itor.where->prev = curr; // re-assign pointers
         return iterator(curr);
      }
   }

   node* curr = new node(anchor(), itor.where->prev, pair);
   itor.where->prev->next = curr;
   itor.where->prev = curr;
   return iterator(curr);
}


//
// listmap::find(const key_type&)
//
template <typename Key, typename Value, class Less>
typename listmap<Key,Value,Less>::iterator
listmap<Key,Value,Less>::find (const key_type& that) {
   TRACE ('l', that);
   iterator itor = begin();
   while(itor != end()){
      if(itor->first == that){
         //TRACE ('f', "Found: " << itor->first);
         break;
      }
      ++itor;
   }
   //if (itor == end()) TRACE ('f', "Didn't find " << that)
   return itor;
}

//
// iterator listmap::erase (iterator position)
//
template <typename Key, typename Value, class Less>
typename listmap<Key,Value,Less>::iterator
listmap<Key,Value,Less>::erase (iterator position) {
   TRACE ('s', &*position);
   iterator itor = position;
   ++itor;

   position.erase();

   return itor;
}

template <typename Key, typename Value, class Less>
void listmap<Key,Value,Less>::iterator::erase(){
	if (where == nullptr) return;
   
	where->next->prev = where->prev;
	where->prev->next = where->next;

	delete where;
}



//
/////////////////////////////////////////////////////////////////
// Operations on listmap::iterator.
/////////////////////////////////////////////////////////////////
//

//
// listmap::value_type& listmap::iterator::operator*()
//
template <typename Key, typename Value, class Less>
typename listmap<Key,Value,Less>::value_type&
listmap<Key,Value,Less>::iterator::operator*() {
   TRACE ('l', where);
   return where->value;
}

//
// listmap::value_type* listmap::iterator::operator->()
//
template <typename Key, typename Value, class Less>
typename listmap<Key,Value,Less>::value_type*
listmap<Key,Value,Less>::iterator::operator->() {
   TRACE ('l', where);
   return &(where->value);
}

//
// listmap::iterator& listmap::iterator::operator++()
//
template <typename Key, typename Value, class Less>
typename listmap<Key,Value,Less>::iterator&
listmap<Key,Value,Less>::iterator::operator++() {
   TRACE ('l', where);
   where = where->next;
   return *this;
}

//
// listmap::iterator& listmap::iterator::operator--()
//
template <typename Key, typename Value, class Less>
typename listmap<Key,Value,Less>::iterator&
listmap<Key,Value,Less>::iterator::operator--() {
   TRACE ('l', where);
   where = where->prev;
   return *this;
}


//
// bool listmap::iterator::operator== (const iterator&)
//
template <typename Key, typename Value, class Less>
inline bool listmap<Key,Value,Less>::iterator::operator==
            (const iterator& that) const {
   return this->where == that.where;
}

//
// bool listmap::iterator::operator!= (const iterator&)
//
template <typename Key, typename Value, class Less>
inline bool listmap<Key,Value,Less>::iterator::operator!=
            (const iterator& that) const {
   return this->where != that.where;
}

