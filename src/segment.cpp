#include "segment.hpp"

#include <string.h>


namespace tumopp{
    Segment::Segment(char allele[], unsigned cn_id_tail_){
        strcpy(allele_origin_, allele);
        id_ = cn_id_tail_;
    }

    
    void Segment::accumulate_point_mutation(unsigned snv_id_tail_){
        snvs_.push_back(snv_id_tail_);
    }
}