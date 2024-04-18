#include <iostream>
#include "defs.h"
#include "Record.h"

typedef unsigned int RunId;
typedef unsigned int RecordId;
typedef int Level;

template <typename Compare>
class LoserTree {
    Level const height;
    Index_r heap;
    const Compare cmp;
    public:
        LoserTree(Level const h, const Compare& cmp, Index_r& index)
        : height(h), heap(index), cmp(cmp) {
            TRACE(true);
            uint32_t cap = capacity();
            for(uint32_t i =0; i<cap; i++){
                //MergeInd temp = {cap + 1, early_fence(i)};
                heap[i].record_id = early_fence(i);
                heap[i].run_id = cap + 1;
            }
        }
        ~LoserTree(){
            // delete heap;
        }

        bool less(MergeInd &a, MergeInd &b){
            bool achanged = false;
            bool bchanged = false;
            if(a.record_id == late_fence(a.run_id) || a.record_id==early_fence(a.run_id)) {
                return false;
            } else {
                achanged = true;
            }
            
                
            
            if(b.record_id == late_fence(b.run_id) || b.record_id==early_fence(b.run_id)) {
                return true;
            } else {
                bchanged = true;
            }
            
            if(achanged) {
                a.record_id = a.record_id - capacity();
            }
            if(bchanged) {
                b.record_id = b.record_id - capacity();
            }
            


            bool compare  = cmp(a, b);
            if(achanged) {
                a.record_id = a.record_id + capacity();
            }
            if(bchanged) {
                b.record_id = b.record_id + capacity();
            }
            return compare;

        }

        RunId capacity() {
            return RunId(1<<height);
        }
        RunId root() {
            return RunId(0);
        }
        void leaf(RunId index, RunId &slot){
            slot = capacity() + index;
        }
        void parent(RunId &slot){
            slot/=2;
        }
        void leaf(RunId index, RunId &slot, Level &level){
            level = 0;
            leaf(index, slot);
        }
        void parent(RunId &slot, Level &level){
            ++level; parent(slot);
        }
        RecordId early_fence(RunId index) {
            return RecordId(index);
        }
        RecordId late_fence(RunId index) {
            return ~RecordId(index);
        }
        void pass(RunId index, RecordId key) {
            MergeInd candidate = {index, key};
            RunId slot;
            for(slot = capacity() + index; (slot /= 2) != 0;) {
                if(heap[slot].run_id > capacity()) {
                    heap[slot] = candidate;
                    return;
                }
                if(less(heap[slot], candidate)) {
                    MergeInd temp = heap[slot];
                    heap[slot] = candidate;
                    candidate = temp;
                }
            }
            heap[root()] = candidate;
        }

        bool empty() {
            MergeInd &hr = heap[root()];
            while(hr.record_id == early_fence(hr.run_id))
                pass(hr.run_id, late_fence(hr.run_id)); 
            return hr.record_id == late_fence(hr.run_id);
        }

        MergeInd poptop(bool invalidate) {
            // if(empty()) 
            //     return nullptr;
            if(invalidate) {
                MergeInd x = heap[root()];
                MergeInd popped = {x.run_id, x.record_id};
                popped.record_id = popped.record_id - early_fence(capacity());
                std::cout<<"popped key - "<<heap[root()].record_id << "run id = "<< popped.run_id<<std::endl;
                heap[root()].record_id = early_fence(heap[root()].run_id);
                return popped;
            }
        }
        MergeInd top() {
            return poptop(false);
        }

        MergeInd pop() {
            return poptop(true);
        }
        void push(RunId index, RecordId key) {
            pass(index, early_fence(capacity()) + key);
        }
        void insert(RunId index, RecordId key) {
            push(index, key);
        }
        void update(RunId index, RecordId key) {
            push(index, key);
        }
        void deleteRecordId(RunId index) {
            pass(index, late_fence(index));
        }

};

// int main() {
//     std::cout<<"Ranjitha"<<std::endl;
//     Level level(2);
//     auto cmp = [](MergeInd const &a, MergeInd const &b) {
//     return a.record_id > b.record_id;
//   };
//     LoserTree ltre(level, cmp);
//     for(int i=0; i<4; i++) {
//         ltre.insert(i, 4-i);
//     }

//     while(!ltre.empty()){
//         std::cout<<ltre.pop()<<std::endl;
//     }
// }


