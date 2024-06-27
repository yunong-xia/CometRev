#include "tissue.hpp"

namespace tumopp{

    class PointMutation{
        public:
            PointMutation(unsigned cn_id_tail_);
            PointMutation copy();
            const unsigned& id() const noexcept {return id_;}

        private:
            //static unsigned curr_highest_id_;
            unsigned id_;
    };

}