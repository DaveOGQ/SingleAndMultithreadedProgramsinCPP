// -------------------------------------------------------------------------------------
// this is the only file you need to edit
// -------------------------------------------------------------------------------------
//
// (c) 2023, Pavol Federl, pfederl@ucalgary.ca
// Do not distribute this file.

#include "fatsim.h"
#include <iostream>
#include <deque>




// Each files is represented by the index and the value stored at the index is file being pointed to by the file represented by the index.
// -1 represents a null pointer and the end of a file.
std::vector<long> fat_check(const std::vector<long> & fat)
{ //create adjcency list 
  std::vector<std::vector<long>> adj;
  //nodes with the current longest count
  std::vector<long> longestList; 
  //current longest count
  long longest = 0;
  //stack for processing nodes, with each notde and its count
  std::deque<std::pair<long, long>> nodes;
  //used to initailize a vector of empty vectors
  std::vector<long> empty; 

  //inintialize adj
  for(long i=0; i <= (long) fat.size(); i++ ){
    adj.push_back(empty);
  }

  //fill in the adjency list for the fat table
  //each index will represent  a node of value index+1, index 0 of the graph will be for -1
  for(long i=0;  i < (long) fat.size(); i++){
    // long NodeThatsPointing = i;
    // long NodePointedTo = fat[i];
    long index = fat[i]+1;
    adj[index].push_back(i);  
  }

  //starts of the stack with node -1 and count 0
  nodes.push_back({-1, 0});

  while(1){
    if(nodes.empty()){//allnodes have been processes
      break;
    }

    auto currentNode = nodes.front();
    auto currentCount = currentNode.second;
    nodes.pop_front();
    
    //get neighboring pointers of the current node
    auto neighboringNodes = adj[currentNode.first + 1];
    
    //updates either the longest count or the longest list
    if(currentCount == longest){
      //if the current count is the same as the longest this node is one of the longest
      longestList.push_back(currentNode.first);
    } else if (currentCount > longest) {
      //if a new largest count has been found update the longest count 
      //then clear longest list and add this node to list
      longestList.clear();
      longest = currentCount;
      longestList.push_back(currentNode.first);
    }

    //push neighboring nodes into the stack
    if(!neighboringNodes.empty()) {
      for(auto neighbor : neighboringNodes ){
        nodes.push_back({neighbor, currentCount+1});
      }
    }
  }

  //if 0 is the longest count remove -1 from the longest list 
  if(longestList.size() == 1){
    if(longestList[0] == -1){
      longestList.clear();
    }
  }
  return longestList;
}

