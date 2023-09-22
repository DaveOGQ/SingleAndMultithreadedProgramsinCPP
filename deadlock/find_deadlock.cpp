//Author:David Oti-George

#include "find_deadlock.h"
#include "common.h"
#include <stdio.h>
#include <iostream>
#include <algorithm>

///
/// parameter edges[] contains a list of request- and assignment- edges
///   example of a request edge, process "p1" resource "r1"
///     "p1 -> r1"
///   example of an assignment edge, process "XYz" resource "XYz"
///     "XYz <- XYz"
///
/// You need to process edges[] one edge at a time, and run a deadlock
/// detection after each edge. As soon as you detect a deadlock, your function
/// needs to stop processing edges and return an instance of Result structure
/// with 'indexl' set to the indexl that caused the deadlock, and 'procs' set
/// to contain names of processes that are in the deadlock.
///
/// To indicate no deadlock was detected after processing all edges, you must
/// return Result with indexl=-1 and empty procs.
///
std::vector<std::string> rprocs; //remaining processes

class Graph {
    public:
    std::vector<std::vector<int>> adj_list; //each index represents a process (a) and each array at each index represents the processs' (b) pointing to (a)
    std::vector<int> out_counts; //number of processes being pointed to by the process at this index
}Graph_g;

Graph g;

std::unordered_map<int, std::string> procNames;//process/resource names and their integer representations

//topological sort 
int topsort(){
    std::vector<int> o_c = g.out_counts;
    
    //get zeros
    std::vector<int> zeros;
    for(int i= 0; i < (int) o_c.size(); i++){
        //if process i has an outcount of 0 add to zeros
        if(o_c.at(i)==0){
            zeros.push_back(i);
        }
    }

    //delete zeros and their connections until there are no zeros left
    while(zeros.size() > 0){
        int n = zeros.back();
        zeros.pop_back();
        for(auto i : g.adj_list.at(n)){//for each process/resource that points to n

            o_c.at(i)--;//subtract this current zero from process/resource i's outcounts
            if(o_c.at(i)== 0){// if i is now a zero add it to the zeros array
                zeros.push_back(i);
            }
            
        }
    }


    //check for process' with an outcount greater than 0,
    for(int i= 0; i < (int) o_c.size(); i++){
        if(o_c[i] > 0){
            std::string str = procNames[i];
            if(str.back() == '.'){//checks that its a process
                    str.pop_back();
                    rprocs.push_back(str);
                }
        }
    }

    if(rprocs.size() > 0){
        return 1;
    }//if anything is added to r rpocs there was a deadlock and the function returns 1

    return 0;//0 otherwise
}



Result find_deadlock(const std::vector<std::string> & edges)
{
    Result result;
    Word2Int w;

    for(int i = 0; i < (int) edges.size(); i++){
        //parse edge
        std::vector<std::string> e = split(edges[i]);
        e[0] = e[0] + "."; //marker to indicate a process or resource, in the eventuality that both a process and resource have the same name
        long unsigned int x = w.get(e[0]);
        long unsigned int y = w.get(e[2]);
        

        //if x or y is less than the cuurrent size of the map then it must be in the map/?

        
       
        //insert edge into graph
        if(e[1] == "<-"){
            //assignment edge (x <- y)

            //check if process already exists in the graph
            
            //adds the process if its not already in the graph
            if(x == g.adj_list.size()) {//if process is new, its integer representation and future index will be equal to size of the adj list.
                procNames.emplace(x, e[0]);
                std::vector<int> newVector1;
                //sets up a new adj list, with y as a resourse that points to it
                newVector1.push_back(y);
                g.adj_list.push_back(newVector1);
                //initializes outcounts with 0
                g.out_counts.push_back(0);
            }else{// if the index of the given process is less than the size of the graph its being represented in the graph already
                g.adj_list.at(x).push_back(y);
            }

            if(y == g.adj_list.size()) {//if process is new, its integer representation and future index will be equal to size of the adj list.
                // if it does not exist in the graph add it to the graph along with its corresponding resource
                // set its out count to 0
                procNames.emplace(x, e[2]);

                //sets up new adj list although nothing points to it yet.
                std::vector<int> newVector2;
                g.adj_list.push_back(newVector2);
                g.out_counts.push_back(1);//beacuse it points to process x above
            }else{
                //if the process already exists update its info, increases outcount by one because it connects to process x
                g.out_counts.at(y)++;
            }

            

        } else if (e[1] == "->"){
            //request edge (x -> y)

            //check if process already exists in the graph
            if( x == g.adj_list.size() ) {//if process is new, its integer representation and future index will be equal to size of the adj list.
                // if it does not exist in the graph add it to the graph along with its corresponding resource
                procNames.emplace(x, e[0]);
                std::vector<int> newVector1;
                g.adj_list.push_back(newVector1);
                // set its out count to 0
                g.out_counts.push_back(1);
            }else{
                //if the process already exists update its outcount as its additionally points to y
                g.out_counts.at(x)++;
            }

            //adds the resource if its not already in the graph
            if(y == g.adj_list.size()) {//if process is new, its integer representation and future index will be equal to size of the adj list.
                procNames.emplace(x, e[2]);
                //sets up new adj list with x as the first process/resource pointing to it
                std::vector<int> newVector2;
                newVector2.push_back(x);
                g.adj_list.push_back(newVector2);
                //sets the out count as 0 since y doesn't point to anything just yet
                g.out_counts.push_back(0);
            }else{
                g.adj_list.at(y).push_back(x);//adds x to adj list and it now point to it
            }
        }

       

        //run top sort
        int finished = topsort();
        //1 is returned if topsort did not finish    
        if(finished == 1){
            result.index = i;
            result.procs = rprocs;
            return result;
            //return procs, and indexl...
        }
    }
    //topsort always finished so there are no deadlocks
    result.index = -1;    
    return result;
}





