//Author:David Oti-George

#include "scheduler.h"
#include "common.h"
#include <deque>
#include <iostream>



// input:
//   quantum = time slice
//   max_seq_len = maximum length of the reported executing sequence
//   processes[] = list of process with populated IDs, arrivals, and bursts
// output:
//   seq[] - will contain the execution sequence but trimmed to max_seq_len size
//         - idle CPU will be denoted by -1
//         - other entries will be from processes[].id
//         - sequence will be compressed, i.e. no repeated consecutive numbers
//   processes[]
//         - adjust finish_time and start_time for each process
//         - do not adjust other fields
//

//calcutlates the amounnt of times the quataum can the skipped, by checking if all processes in current stack of processes in the rq can safely execute 1 or more bursts
int timeSkip (std::vector<Process> & processes, int64_t curr_time, std::deque<int> & rq, std::deque<int> & jq, std::deque<int64_t>  r_bursts, int64_t quantum ) {//if it returns 0 the time skip cant be done
    
    int i = 0;

    for(auto n : rq){
        //a process has not started yet so time cannot be skipped
        if(processes[n].start_time == -1){
            return i;
        }
    }

    if(!jq.empty()){//accounts for possible arrivals
        int64_t nextArrival = processes[jq.front()].arrival;//checks the arrival time of the next process

        while(1){
            //decrements the bursts by the quantuam and updates the current time by the time each burst uses
            for(int l = 0; l < (int) rq.size(); l++){
                curr_time += quantum;
                r_bursts[l] -= quantum;
            }
            //checks is any bursts are less than 0
            for(auto r : r_bursts){
                if(r < 0){//r<=0
                    //if a burst is less than 0 when the quatuam is decremented from it, the time skip cannot be done safely
                    return i;
                }
            }

            if(curr_time > nextArrival){
                return i;//if the current time exceeds the next arrival time the skip cannot be done safely, because ariving processes need to be inserted frist
            }

            i++;// if all safety checks pass the entire rq stack can excute one more burst 
           
        }
    } else {//same checks but with no worry of an arrival time

        while(1){
            for(int l = 0; l < (int) rq.size(); l++){
                curr_time += quantum;
                r_bursts[l] -= quantum;
            }

            for(auto r : r_bursts){
                if(r < 0){ //r<=0
                    return i;
                }
            }

            i++;
        }
        
    }

    
    return i;
}

//Similation function
void simulate_rr(
    int64_t quantum, 
    int64_t max_seq_len,
    std::vector<Process> & processes,
    std::vector<int> & seq
) {
    std::deque<int> rq, jq; //contains the id of the processes
    int64_t curr_time = 0;
    std::deque<int64_t> r_bursts; //each index corresponds to an index in rq and its remaining  burst

    //put all the porcesses in the job queue
    for(auto p : processes){
        jq.push_back(p.id);
    }

    //set the last id to the first id see, this is a tracker that compares every current id 
    //with the previous one and adds in to the seq[] if its id is different from the last id
    

    seq.clear();
    r_bursts.clear();
    //adds the first id
    seq.push_back(-1);


    int i, k;//used to access processes as these indices,
    int64_t r_burst, r_burst2, last_arrival; //tracks the remaning burst for 2 differenct processes, and the arivaltime of a previous process
    
    Process *p;
    Process *q;

    int skipTime = 0;

    while(1){
        if(rq.empty() && jq.empty()) break;

        if(!rq.empty()){
            //function check for full time skip
            skipTime = timeSkip(processes, curr_time, rq, jq, r_bursts, quantum);
        }

        if(rq.empty()){//process the job queue if ready queue is empty
            //first element of jq
            if(seq.back() != -1){// if this is a different id from the last put it in seq[]
                seq.push_back(-1);
            }

            i = jq.front();

            p = & processes[i];

            curr_time = p->arrival;
            last_arrival = curr_time;
            while(1){
                if(p->arrival == last_arrival){//this while loop will grab all processes with the same arival time as the last arrival
                    jq.pop_front(); //removes the front process only after its been confrimed to have the same arival time as the last arival

                    rq.push_back(p->id); //puts the process into rq
                    r_bursts.push_back(p->burst);// puts the process' burst into r_bursts 

                    if(!jq.empty()){//if there is another process in jq
                        i = jq.front(); 
                        p = & processes[i]; //get the process and on the next iteration of the while loop it will only continue if it has the same arival time
                    }else {
                        break; //break if jq is empty
                    }
                } else { 
                    break;
                }
            }
        } else if (skipTime != 0){
            //skip by number of times the whole rq can be processed
            curr_time += skipTime * rq.size() * quantum;

            for(int l = 0; l < (int) rq.size(); l++){
                //updates the remaining bursts for all processes on the queue
                r_bursts[l] -=  quantum * skipTime;
            }

            for(int i=1; i <= skipTime; i++){//updates seq while accounting for the timeskip
                for(auto r : rq){
                    if(seq.back() != r){
                       seq.push_back(r); 
                    }
                }
            }

            skipTime = 0;//reset time skip
        } 
        else {//rq is not empty
            //first element of rq
            i = rq.front();
            rq.pop_front();
            p = & processes[i];

            r_burst = r_bursts.front();
            r_bursts.pop_front();

            if(p->burst == r_burst){//checks if burst is being executed for the first time
                p->start_time = curr_time;
            }

            if(p->id != seq.back()){// if this is a different id from the last put it in seq[]
                seq.push_back(p->id);
            }
            

            int pushed = 0;//flag that signifies that the current process p has been pushed into rq and doestn need to be pushed again
            if(r_burst > quantum){//if the remaining burst is greater than the quatum adjust remaining burst and curr time by one quantum
                r_burst = r_burst - quantum;
                curr_time += quantum;
                
                if(!jq.empty()){
                    k = jq.front();
                    q = & processes[k];//gets a second process at the front of jq
                    int64_t r_burst2 = q->burst;


                    while(1){//continues to get processes so long as they arrived within the time that the current process was run on the cpu
                        if(q->arrival <= curr_time){
                            
                            if(pushed == 0 && q->arrival == curr_time){
                                rq.push_back(p->id);//puts the current process into the queue, after the ones that arrived before it ended, but before the latest process that arrives 'now' or at current time
                                r_bursts.push_back(r_burst);
                                pushed = 1;
                            }

                            rq.push_back(q->id);
                            r_bursts.push_back(r_burst2);


                            jq.pop_front();//q can be safely removed after the checks have been done and the needed data has been transferred
                            if(!jq.empty()){
                                //get next job
                                k = jq.front();
                                q = & processes[k];
                                r_burst2 = q->burst;
                            } else{
                                break;//no more jobs in jq
                            }
                        }else {//no job has arrived yet so cpu will be idle
                            break;
                        }
                    }
                }

                if(pushed == 0){
                    rq.push_back(p->id);//puts the current process into the queue, after the ones that arrived in the duration of the quantum were inserted, if it hasnt been put in already
                    r_bursts.push_back(r_burst);
                }
            } else {//rburst is less than or equal to quatuam
                //process will end here and the finish time is updated
                curr_time = curr_time + r_burst;
                p->finish_time = curr_time;

                if(!jq.empty()){
                    k = jq.front();
                    q = & processes[k];//gets a second process at the front of jq
                    r_burst2 = q->burst;

                    while(1){
                        if(q->arrival <= curr_time){//if current time is past the arrival time  of process at the front of jq, simply insert that process into rq
                            

                            rq.push_back(q->id);
                            r_bursts.push_back(r_burst2);


                            jq.pop_front();//q can be safely removed after the checks have been done and the needed data has been transferred
                            if(!jq.empty()){
                                //get next job
                                k = jq.front();
                                q = & processes[k];
                                r_burst2 = q->burst;
                            } else{
                                break;//no more jobs in jq
                            }
                        }else {
                            break;//no job has arrived yet so cpu will be idle
                        }
                    }
                }



            }
        }    
    }



}
