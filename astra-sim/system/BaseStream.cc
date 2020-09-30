/******************************************************************************
Copyright (c) 2020 Georgia Institute of Technology
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Author : Saeed Rashidi (saeed.rashidi@gatech.edu)
*******************************************************************************/

#include "BaseStream.hh"
#include "StreamBaseline.hh"
namespace AstraSim{
    std::map<int,int> BaseStream::synchronizer;
    std::map<int,int> BaseStream::ready_counter;
    std::map<int,std::list<BaseStream *>> BaseStream::suspended_streams;
    void BaseStream::changeState(StreamState state) {
        this->state=state;
    }
    BaseStream::BaseStream(int stream_num,Sys *owner,std::list<CollectivePhase> phases_to_go){
        this->stream_num=stream_num;
        this->owner=owner;
        this->initialized=false;
        this->phases_to_go=phases_to_go;
        if(synchronizer.find(stream_num)!=synchronizer.end()){
            synchronizer[stream_num]++;
        }
        else{
            //std::cout<<"synchronizer set!"<<std::endl;
            synchronizer[stream_num]=1;
            ready_counter[stream_num]=0;
        }
        for(auto &vn:phases_to_go){
            if(vn.algorithm!=NULL){
                vn.init(this);
            }
        }
        state=StreamState::Created;
        preferred_scheduling=SchedulingPolicy::None;
        creation_time=Sys::boostedTick();
        total_packets_sent=0;
        current_queue_id=-1;
        priority=0;
    }
    void BaseStream::declare_ready(){
        ready_counter[stream_num]++;
        if(ready_counter[stream_num]==owner->total_nodes){
            synchronizer[stream_num]=owner->total_nodes;
            ready_counter[stream_num]=0;
        }
    }
    bool BaseStream::is_ready(){
        return synchronizer[stream_num]>0;
    }
    void BaseStream::consume_ready(){
        //std::cout<<"consume ready called!"<<std::endl;
        assert(synchronizer[stream_num]>0);
        synchronizer[stream_num]--;
        resume_ready(stream_num);
    }
    void BaseStream::suspend_ready(){
        if(suspended_streams.find(stream_num)==suspended_streams.end()){
            std::list<BaseStream*> empty;
            suspended_streams[stream_num]=empty;
        }
        suspended_streams[stream_num].push_back(this);
        return;
    }
    void BaseStream::resume_ready(int st_num){
        if(suspended_streams[st_num].size()!=(owner->all_generators.size()-1)){
            return;
        }
        int counter=owner->all_generators.size()-1;
        for(int i=0;i<counter;i++){
            StreamBaseline *stream=(StreamBaseline*)suspended_streams[st_num].front();
            suspended_streams[st_num].pop_front();
            owner->all_generators[stream->owner->id]->proceed_to_next_vnet_baseline(stream);
        }
        if(phases_to_go.size()==0){
            destruct_ready();
        }
        return;
    }
    void BaseStream::destruct_ready() {
        std::map<int,int>::iterator it;
        it=synchronizer.find(stream_num);
        synchronizer.erase(it);
        ready_counter.erase(stream_num);
        suspended_streams.erase(stream_num);
    }
}