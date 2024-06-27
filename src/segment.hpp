#include "pointmutation.hpp"

#include <string>
#include <unordered_set>
#include <map>

namespace tumopp
{
    class Segment{
        public:
            Segment(char allele[], unsigned cn_id_tail_); // either 'paternal' or 'maternal'

            Segment clone();

            void accumulate_point_mutation(unsigned snv_id_tail_);

        private:            
            char allele_origin_[10];  // paternal or maternal
            
            unsigned id_;

            struct hash_point_mutation_id {
                //! ruping: hash function for cell ids: for tracking dead cells 
                unsigned operator() (const std::shared_ptr<tumopp::PointMutation>& x) const noexcept {
                    std::hash<unsigned> hash_id;
                    return hash_id(x->id());
                }
            };

            struct equal_point_mutation_id {
                //! ruping Compare cell ids: for tracking dead cells
                bool operator() (const std::shared_ptr<tumopp::PointMutation>& lhs,
                                const std::shared_ptr<tumopp::PointMutation>& rhs) const noexcept {
                return lhs->id() == rhs->id();
                }
            };    

            std::unordered_set<
                std::shared_ptr<PointMutation>,
                hash_point_mutation_id,
                equal_point_mutation_id> snvs_;

    };



} // namespace tumopp
