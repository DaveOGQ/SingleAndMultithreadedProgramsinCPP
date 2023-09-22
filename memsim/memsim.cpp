//Author: David Oti-George
#include "memsim.h"
#include <cassert>
#include <iostream>
#include <list>
#include <unordered_map>
#include <set>
#include <cmath>
#include <algorithm>

struct Partition {
  int tag;
  int64_t size, addr;
};
  
typedef std::list<Partition>::iterator PartitionRef;

struct scmp {//stores partitions in terms of size first and address second
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const {
  if (c1->size == c2->size)
    return c1->addr < c2->addr;
  else
    return c1->size > c2->size;
}
};

struct Simulator {
  public: 
    int64_t p_size;//page size
    // all partitions, in a linked list
    std::list<Partition> all_blocks;
    // quick access to all tagged partitions
    std::unordered_map<long, std::vector<PartitionRef>> tagged_blocks;
    // sorted partitions by size/address
    std::set<PartitionRef,scmp> free_blocks;
    int numofPageRequests = 0;
  Simulator(int64_t page_size)
  {
    // constructor
    p_size = page_size;
    //first block
    Partition firstPartition;
    firstPartition.tag = 0;
    firstPartition.addr = 0;
    firstPartition.size = 0;

    all_blocks.push_back(firstPartition);//starts simulation with a free block with size 0
    free_blocks.insert(all_blocks.begin());
  }

  void addToTaggedBlocks(long tag, PartitionRef ref){
    tagged_blocks[tag].push_back(ref);
  }


  void wholeTagonNewpage(int tag, int64_t size){
    //pages added completely dependant on size get the number of pages, 
    //add a block with the allocated space, and add a freeblock with the remainging block
    float div = (float) size / float (p_size);
    int64_t numofpages = ceil(div);
    numofPageRequests += numofpages;
    int64_t addedmemory = numofpages * p_size;
    int64_t memoryforfreeBlock = addedmemory - size;
    //get element  on the end of the list
    PartitionRef lastelement = std::prev(all_blocks.end());
    Partition &last = *lastelement;

    if(last.tag == tag){
      //combine them together by simply editting the previous tag and then just adding the remaining free spcae
      last.size += size;
      if(memoryforfreeBlock > 0 ){//if there unused memory insert it as a free block
        Partition newFreeBlock = {0, memoryforfreeBlock, last.addr+last.size};
        all_blocks.push_back(newFreeBlock);
        PartitionRef newFreeBlockRef = std::prev(all_blocks.end());
        free_blocks.insert(newFreeBlockRef);
      }
    }else{//add a new block with the given tag 
      //get the new address by doing last + size
      Partition newTaggedBlock = {tag, size, last.addr+last.size};
      all_blocks.push_back(newTaggedBlock);
      PartitionRef  newTaggedRef = std::prev(all_blocks.end());
      
      addToTaggedBlocks(tag, newTaggedRef);

      if(memoryforfreeBlock > 0 ){//if there unused memory insert it as a free block
        PartitionRef newlastelement = std::prev(all_blocks.end());
        Partition newlast = *newlastelement;
        Partition newFreeBlock = {0, memoryforfreeBlock, newlast.addr+newlast.size};
        all_blocks.push_back(newFreeBlock);
        PartitionRef newFreeBlockRef = std::prev(all_blocks.end());
        free_blocks.insert(newFreeBlockRef);
      }
    }
  }

  void partTagonLastpage(int tag, int64_t size){
    PartitionRef lastelement = std::prev(all_blocks.end());
    Partition &last = *lastelement;
    last.tag = tag;//change this free block into this tagged block
    int64_t newsize = size - last.size;
    PartitionRef secondlastelementRef = std::prev(lastelement);
    Partition &secondlastelement = *secondlastelementRef;

    if(lastelement != all_blocks.begin()){
      
      if(secondlastelement.tag != tag){
        addToTaggedBlocks(tag, lastelement);
      } else {
        secondlastelement.size += last.size;//increase the size of the previous block and then just get rid of this free block
        all_blocks.erase(lastelement);
      }
    } else{//for the set up when there are no blocks simply cahnges the tag of the first block then requests more pages below
      addToTaggedBlocks(tag, lastelement);
    }
    
    free_blocks.erase(lastelement);// remove this once free block from free_blocks
    wholeTagonNewpage(tag, newsize);
  
    //change the free tag to take a portion of size then pass the remainder into the upper function and it will add the required pages and combine the remaining memmory to the one we just added
    //remove the free tag from freeblocks
  }

  

  void insertpartialLeftNeighbor(int tag, int64_t size, PartitionRef topFreeBlockRef){
    //when a free block isnt fully used insert the tagged block in front of the free block, 
    //adjust the free block's size then merge the new tagged block with its left neighbor if necessary
    PartitionRef leftNeighbor = std::prev(topFreeBlockRef);
    if(leftNeighbor->tag == tag){//combine together
      leftNeighbor->size += size;
      topFreeBlockRef->size -= size;
      topFreeBlockRef->addr = leftNeighbor->size + leftNeighbor->addr;
      free_blocks.insert(topFreeBlockRef);

    } else {//insert new node and readjust the current free one then reinsert into free block
      Partition blockInsert = {tag, size, leftNeighbor->size+leftNeighbor->addr};
      all_blocks.insert(std::next(leftNeighbor),blockInsert);
      PartitionRef blockInsertRef = std::next(leftNeighbor);

      addToTaggedBlocks(tag, blockInsertRef);

      topFreeBlockRef->size -= size;
      topFreeBlockRef->addr = blockInsert.size+blockInsert.addr;
      free_blocks.insert(topFreeBlockRef);
    }
  }

  void insertPartialNoLeftNeighbor(int tag, int64_t size, PartitionRef topFreeBlockRef){
    //if the partial freeblock being used up is the first block it will have no 
    //left neighbor hence just insert the new block at the front of the list and adjust accordingly
    Partition blockInsert = {tag, size, 0};
    all_blocks.push_front(blockInsert);
    PartitionRef blockInsertRef = all_blocks.begin();

    addToTaggedBlocks(tag, blockInsertRef);

    topFreeBlockRef->size -= size;
    topFreeBlockRef->addr = size;
    free_blocks.insert(topFreeBlockRef);

  }
  
  PartitionRef merge(PartitionRef Left, PartitionRef Right){//merges 2 blocks together then returns the new merged block
    Left->size += Right->size;
    all_blocks.erase(Right);
    return(Left);
  }

  void eraseTag(long tag, PartitionRef tagToErase){//used to get rid of right blocks that wont be overwritten by left ones, removes a tagged block from tagged blocks
    tagged_blocks.at(tag).erase(std::remove(tagged_blocks.at(tag).begin(), tagged_blocks.at(tag).end(), tagToErase), tagged_blocks.at(tag).end());
  }

  void insertTag(int tag, int64_t size){
    PartitionRef topFreeBlockRef = *free_blocks.begin();
    free_blocks.erase(topFreeBlockRef);

    if(topFreeBlockRef->size == size){
      topFreeBlockRef->tag = tag;
      //this top free block will not be reinserted into free blocks
      //check Both neighbors some merging required
      //create a merge function that returns the new merged block(merge from left to right)
      if(topFreeBlockRef == all_blocks.begin()){//front (possible merge with right)
        PartitionRef right = std::next(topFreeBlockRef);
        
        if(right->tag == tag){
          // merge from right to left with prev + free
          //need to delete the reference to right from tagged blocks
          
          eraseTag(tag, right);//right is going to get deleted from all blocks in the merge so we ned to remove it from tagged blocks as well

          PartitionRef newTaggedBlock = merge(topFreeBlockRef, right);
          addToTaggedBlocks(tag, newTaggedBlock);//topFreeBlockRef is not in tagged blocks yet so when we override right we need to insert it into tagged blocks
        }else{
          //no merging simply change this free blocks tag and add it to tagged blocks
          
          addToTaggedBlocks(tag, topFreeBlockRef);
        }
      } else if(topFreeBlockRef == std::prev(all_blocks.end())){ //(possible merge with left)
        PartitionRef left = std::prev(topFreeBlockRef);
        if(left->tag == tag){
          // merge from left to right with prev + free
          merge(left,topFreeBlockRef);
        }else{
          //no merging simply change this free blocks tag and add it to tagged blocks
          
          addToTaggedBlocks(tag, topFreeBlockRef);
        }
      } else{ //possible merge left and right
        PartitionRef left = std::prev(topFreeBlockRef);
        PartitionRef right = std::next(topFreeBlockRef);
        if(left->tag == tag && right->tag == tag){
          // merge from left to right with prev + free, then that result + next
          eraseTag(tag, right);//right is going to get deleted from all blocks in the merge so we ned to remove it from tagged blocks as well
          PartitionRef newLeft = merge(left,topFreeBlockRef);
          merge(newLeft, right);
        } else if (left->tag == tag && right->tag != tag){
          //merge only left and right prev + free
          merge(left, topFreeBlockRef);
        } else if (left->tag != tag && right->tag == tag){
          //merge only left and right prev + free
          eraseTag(tag, right);//right is going to get deleted from all blocks in the merge so we ned to remove it from tagged blocks as well

          PartitionRef newTaggedBlock = merge(topFreeBlockRef, right);

          addToTaggedBlocks(tag, newTaggedBlock);//topFreeBlockRef is not in tagged blocks yet so when we override right we need to insert it into tagged blocks
        } else if (left->tag != tag && right->tag != tag){
          //no merging simply change this free blocks tag and add it to tagged blocks
          topFreeBlockRef->tag = tag;
          addToTaggedBlocks(tag, topFreeBlockRef);
        }
      }

    } else {
      //merging with left neighbor,
      
      if(topFreeBlockRef != all_blocks.begin()){//There is a left neighbor
        insertpartialLeftNeighbor(tag, size, topFreeBlockRef);
      } else { //there is no left neighbor
        //just insert the node infront of the free block and then adjust the free block and reinsert it into free_blocks
        insertPartialNoLeftNeighbor(tag, size, topFreeBlockRef);
      }
    
      //if nieghbor as the same tag, merge the two and inserting a new tag is not required
      //otherwise
      //insert the block needed infront of the free space, and assign the free space as its std::next
      //edit the free spcae to be smaller and the remaining space withh now go in the free space
    }
    //will take the the largest free spot, combine with the neighboring free locations and take the free location out of the set and reinsert the reaminder if there is one
    
  }

  // page request // end of the list is free block
  // if the end of the list is a free block, add the necessary space ,by simply extending the free blocks size
  // you need to remove this block from FREEBLOCKS, THEN REINSERT BACK INTO FREE BLOCKS, this block will then be moved to the front of free blocks
  // else add a new block and insert it into free blocks
  
  void allocate(int tag, int64_t size )
  {
    //|1|2|0| 
    if(free_blocks.empty()){
      wholeTagonNewpage(tag,size);
      //add new pages and the tag
    } else {
      auto largestBlockIterator = *free_blocks.begin();//partitionRef 

      if(largestBlockIterator->size >= size ){
        insertTag(tag, size);
      } else {
        //page request 
        auto &lastBlock = *std::prev(all_blocks.end());
        auto lastBlockTag = lastBlock.tag;
        if (lastBlockTag == 0){
          partTagonLastpage( tag,  size);
          //takes care of the little freespace and the end then calls wholeTagonNewpage to add more pages
        }else{
          wholeTagonNewpage(tag,size);
          //adds new pages and the tag
        }
      }
    }
    
    
  }
  
  void deallocate(int tag)
  {
    if(tagged_blocks[tag].size() != 0){
      //if size is one just check if its the same as the tag and just deallocate it and insert into free_blocks!
      if(all_blocks.size() == 1){
        if(all_blocks.begin()->tag == tag){
          all_blocks.begin()->tag = 0;
          free_blocks.insert(all_blocks.begin());
        }
      } else {//size greater than 1
        
        for (PartitionRef iter : tagged_blocks[tag]){
          iter->tag = 0;
          if(iter == all_blocks.begin()){//the tag is at the beginning
            PartitionRef next = std::next(iter);
            if(next->tag == 0){
              free_blocks.erase(next);
              PartitionRef newfreeBlock = merge(iter, next);
              free_blocks.insert(newfreeBlock);
            } else {
              free_blocks.insert(iter);
            }
            //check if next is free
            //just change tag
          } else if (iter == std::prev(all_blocks.end())){//the tag is at the end 
            //check if prev is free
            PartitionRef prev = std::prev(iter);
            if(prev->tag == 0){
              free_blocks.erase(prev);
              PartitionRef newfreeBlock = merge(prev, iter);//merge the free blocks
              free_blocks.insert(newfreeBlock);
            } else {
              free_blocks.insert(iter);//declare block as free
            }
          } else{//the tag is in the middle
            PartitionRef next = std::next(iter);
            PartitionRef prev = std::prev(iter);
            if(next->tag == 0 && prev->tag == 0){// both adjacent tags are free
              free_blocks.erase(prev);
              free_blocks.erase(next);
              PartitionRef newfreeBlock = merge(prev, iter);
              PartitionRef newerfreeBlock = merge(newfreeBlock, next);
              free_blocks.insert(newerfreeBlock);
            } else if(next->tag != 0 && prev->tag == 0){//previous tag is free
              free_blocks.erase(prev);
              PartitionRef newfreeBlock = merge(prev, iter);
              free_blocks.insert(newfreeBlock);
            } else if(next->tag == 0 && prev->tag != 0){//next tag is free
              free_blocks.erase(next);
              PartitionRef newfreeBlock = merge(iter, next);
              free_blocks.insert(newfreeBlock);
            } else {//both adjacent tags are not free so we can insert this one into free blocks
              free_blocks.insert(iter);
            }
            //summary
            //check is prev and next are free
            //or if prev is free and not next
            //or is prev isnt free and next is
            //otherwise just free up this block 
          }
          
        }
      }
      tagged_blocks[tag].clear();//remove this tagged block
    }
  }

  //Debugging function to check memory is being allocated and deallocated correctly
  // void showState(){
  //   //show freeBLocks
  //   //show allblock
  //   //[tag: , size: , addr:]

  //   for(auto memseg : all_blocks){
  //     std::cout<<"["<<"tag:"<<memseg.tag<<" size:"<<memseg.size<<" addr:"<<memseg.addr<<"]"<<std::endl;
  //   }

  //   std::cout<<"------------------------------------------------------"<<std::endl;

  //   for(auto freeblock : free_blocks){
  //     std::cout<<"["<<"tag:"<<freeblock->tag<<" size:"<<freeblock->size<<" addr:"<<freeblock->addr<<"]"<<std::endl;
  //   }

  //   std::cout<<"------------------------------------------------------"<<std::endl;

  //   for(auto tag : tagged_blocks){
  //     std::cout<<tag.first<<": [";
  //     for(auto block : tag.second){
  //       std::cout<<block->addr<<",";
  //     }
  //     std::cout<<"] "<<std::endl;
      
  //   }

  //   std::cout<<"------------------------------------------------------"<<std::endl;
  // }

  MemSimResult getStats()
  {
   
    MemSimResult result;
    PartitionRef free = *free_blocks.begin();
    result.max_free_partition_size = free->size;
    result.max_free_partition_address = free->addr;
    result.n_pages_requested = numofPageRequests;
    return result;
  }
  
};

MemSimResult mem_sim(int64_t page_size, const std::vector<Request> & requests)
{
  Simulator sim(page_size);

  for (const auto & req : requests) {
    if (req.tag < 0) {
      sim.deallocate(-req.tag);
    } else {
      sim.allocate(req.tag, req.size);
    }
    //sim.showState();
  }
  return sim.getStats();
}
