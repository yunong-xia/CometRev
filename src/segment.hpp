
#include <string>
#include <map>
#include <vector>

namespace tumopp
{
    class Segment{
        public:
            Segment(char allele[], unsigned cn_id_tail_); // either 'paternal' or 'maternal'

            Segment clone();

            void accumulate_point_mutation(unsigned snv_id_tail_);

            Segment(const Segment& other) noexcept:
                id_(other.id_),
                snvs_(other.snvs_) {
                    strcpy(allele_origin_, other.allele_origin_);
                }
            
            ~Segment() noexcept = default;

        private:            
            //! paternal or maternal
            char allele_origin_[10];
            
            //! segment id
            unsigned id_;
            
            //! ids' of point mutations
            std::vector<unsigned> snvs_;
    };


} // namespace tumopp
