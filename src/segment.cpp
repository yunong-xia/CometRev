#include "segment.hpp"

#include <string.h>


namespace tumopp{
    Segment::Segment(char allele[], unsigned cn_id_tail_){
        strcpy(allele_origin_, allele);
        id_ = cn_id_tail_;
    }

    
    
}