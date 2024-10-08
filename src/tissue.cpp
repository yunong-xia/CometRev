/*! @file tissue.cpp
    @brief Implementation of Tissue class
*/
#include "tissue.hpp"
#include "benchmark.hpp"

#include <wtl/random.hpp>
#include <wtl/iostr.hpp>
#include <wtl/math.hpp>
#include <wtl/numeric.hpp>
#include <wtl/algorithm.hpp>
#include <wtl/genetic.hpp>

namespace tumopp {

Tissue::Tissue(
  const size_t initial_size,
  const unsigned dimensions,
  const std::string& coordinate,
  const std::string& local_density_effect,
  const std::string& displacement_path,
  const EventRates& init_event_rates,
  const uint_fast32_t seed,
  const uint_fast32_t seed2,
  const uint_fast32_t seed3,
  const uint_fast32_t seed_cna,
  const uint_fast32_t seed_cna_event,
  const uint_fast32_t seed_copy,
  const bool enable_benchmark):
  engine_(std::make_unique<urbg_t>(seed)),
  engine2_(std::make_unique<urbg_t>(seed2)),
  engine3_(std::make_unique<urbg_t>(seed3)),
  engine_cna_(std::make_unique<urbg_t>(seed_cna)),
  engine_cna_event_(std::make_unique<urbg_t>(seed_cna_event)),
  engine_copy_(std::make_unique<urbg_t>(seed_copy)) {
    if (enable_benchmark) {
        benchmark_ = std::make_unique<Benchmark>();
        benchmark_->append(0u);
    }
    snapshots_.precision(std::cout.precision());
    drivers_.precision(std::cout.precision());
    init_coord(dimensions, coordinate);
    init_insert_function(local_density_effect, displacement_path);
    const auto initial_coords = coord_func_->sphere(initial_size);
    const auto origin = std::make_shared<Cell>(
      initial_coords[0], ++id_tail_,
      std::make_shared<EventRates>(init_event_rates)
    );
    extant_cells_.insert(origin);
    while (extant_cells_.size() < initial_size) {
        for (const auto& mother: extant_cells_) {
            const auto daughter = std::make_shared<Cell>(*mother);
            const auto ancestor = std::make_shared<Cell>(*mother);
            ancestor->set_time_of_death(0.0, extant_cells_.size());   //ruping
            mother->set_time_of_birth(0.0, ++id_tail_, ancestor, extant_cells_.size());
            daughter->set_time_of_birth(0.0, ++id_tail_, ancestor, extant_cells_.size());
            daughter->set_coord(initial_coords[extant_cells_.size()]);
            extant_cells_.insert(daughter);
            if (extant_cells_.size() >= initial_size) break;
        }
    }
    for (const auto& cell: extant_cells_) queue_push(cell);
}

Tissue::~Tissue() = default;

void Tissue::init_coord(const unsigned dimensions, const std::string& coordinate) {
    std::unordered_map<std::string, std::unique_ptr<Coord>> swtch;
    swtch["neumann"] = std::make_unique<Neumann>(dimensions);
    swtch["moore"] = std::make_unique<Moore>(dimensions);
    swtch["hex"] = std::make_unique<Hexagonal>(dimensions);
    try {
        coord_func_ = std::move(swtch.at(coordinate));
    } catch (std::exception& e) {
        std::ostringstream oss;
        oss << "\n" << __FILE__ << ':' << __LINE__ << ':' << __PRETTY_FUNCTION__
            << "\nInvalid value for -C (" << coordinate << "); choose from "
            << wtl::keys(swtch);
        throw std::runtime_error(oss.str());
    }
}

bool Tissue::grow(const size_t max_size, const double max_time,
                  const double snapshot_interval,
                  size_t seeding_size,        // ruping
                  size_t recording_early_growth,
                  size_t mutation_timing,
                  bool stopMutH,              // ruping
                  bool verbose) {
    if (recording_early_growth > 0u) {snapshots_append();}
    bool success = false;
    unsigned i_snapshot = 1u;
    double time_snapshot = i_snapshot * snapshot_interval;
    constexpr size_t progress_interval{1 << 12};
    unsigned int seedingSize_cur = 0;
    while (true) {
        auto it = queue_.begin();
        time_ = it->first;
        const auto mother = std::move(it->second);
        queue_.erase(it);
        if (time_ > max_time || extant_cells_.size() >= max_size) {
            success = true;   // maybe not; but want to exit with record
            break;
        }
        if (time_snapshot > 0.0 && time_ > time_snapshot) {
            snapshots_append();
            time_snapshot = ++i_snapshot * snapshot_interval;
        }
        if (mother->next_event() == Event::birth) {

            // ruping: random sampling of seeding cells, and forcing them to be dead
            const auto cur_size = extant_cells_.size();
            if ( cur_size > 0  && cur_size < max_size && cur_size != seedingSize_cur && cur_size % seeding_size == 0 ) {  //sample according to size

               mother->set_time_of_death(time_, extant_cells_.size());
               // ruping: keep the information of the seeding cells
               dead_cells_.insert(mother);

               if (snapshot_interval == 0) { // when plateau
                 std::cout << "yes\tseeding size\t" << seedingSize_cur << std::endl;
               }
               
               // ruping: seeding
               seedingCells_ << mother->seeding(cur_size);
               seedingSize_cur = cur_size;
             
               extant_cells_.erase(mother);
               continue;

            }
          
            const auto daughter = std::make_shared<Cell>(*mother);
            if (insert(daughter)) {                             // if insert is successful
                const auto ancestor = std::make_shared<Cell>(*mother);
                ancestor->set_time_of_death(time_, extant_cells_.size());   //ruping
                mother->set_time_of_birth(time_, ++id_tail_, ancestor, extant_cells_.size());
                daughter->differentiate(*engine_);
                daughter->set_time_of_birth(time_, ++id_tail_, ancestor, extant_cells_.size());
                
                //drivers_ << mother->mutate(*engine_, *engine3_);
                //drivers_ << daughter->mutate(*engine_, *engine3_);

                printf("mutate cnv\n");
                drivers_ << mother ->mutate_cnv(*engine_, *engine3_, *engine_cna_, *engine_cna_event_, *engine_copy_); //Yunong Xia
                drivers_ << daughter ->mutate_cnv(*engine_, *engine3_, *engine_cna_, *engine_cna_event_, *engine_copy_); // Yunong Xia


                // //stopMutH, boolean option to indicate whether or not to stop mutation after some tumor size
                // if (! stopMutH) {                                 //ruping
                //    passengers_ << mother->mutate2(*engine2_, *engine3_);
                //    passengers_ << daughter->mutate2(*engine2_, *engine3_);
                // } else {                                          //ruping
                //      if (cur_size <= max_size*0.5) {
                //     passengers_ << mother->mutate2(*engine2_, *engine3_);
                //     passengers_ << daughter->mutate2(*engine2_, *engine3_);
                //   }
                // }

                //stopMutH, boolean option to indicate whether or not to stop mutation after some tumor size
                if (! stopMutH) {                                 //ruping
                   passengers_ << mother->mutate_snv_on_cn(*engine2_, *engine3_);
                   passengers_ << daughter->mutate_snv_on_cn(*engine2_, *engine3_);
                } else {                                          //ruping
                     if (cur_size <= max_size*0.5) {
                    passengers_ << mother->mutate_snv_on_cn(*engine2_, *engine3_);
                    passengers_ << daughter->mutate_snv_on_cn(*engine2_, *engine3_);
                  }
                }

                if (extant_cells_.size() == mutation_timing) {  //introduce mutation at specific cell
                    mutation_timing = 0u; // once
                    drivers_ << daughter->force_mutate(*engine_);
                }
                
                queue_push(mother);
                queue_push(daughter);
                const auto size = extant_cells_.size();
                if ((size % progress_interval) == 0u) {
                    if (verbose) std::cerr << "\r" << size;
                    if (benchmark_) benchmark_->append(size);
                }
            } else {
                queue_push(mother, true);
                continue;                            // skip write()
            }
            
        } else if (mother->next_event() == Event::death) {
          
            mother->set_time_of_death(time_, extant_cells_.size());  // ruping: re-remember the death time of the dead cell
            dead_cells_.insert(mother);        // ruping: need to keep the information of dead cells
            extant_cells_.erase(mother);
            if (extant_cells_.empty()) break;
            
        } else {
            migrate(mother);
            queue_push(mother);
        }
        if (extant_cells_.size() < recording_early_growth) {
            snapshots_append();
        } else {
            recording_early_growth = 0u;  // prevent restart by cell death
        }
    }
    if (verbose) std::cerr << "\r" << extant_cells_.size() << std::endl;
    return success;
}

void Tissue::plateau(const double time) {
    queue_.clear();
    for (auto& p: extant_cells_) {
        p->increase_death_rate();
        queue_push(p);
    }
    grow(std::numeric_limits<size_t>::max(), time_ + time);
}

void Tissue::treatment(const double death_prob, const size_t num_resistant_cells) {
    const size_t original_size = extant_cells_.size();
    std::vector<std::shared_ptr<Cell>> cells;
    cells.reserve(original_size);
    for (const auto& p: queue_) { // for reproducibility
        cells.emplace_back(p.second);
    }
    std::shuffle(cells.begin(), cells.end(), *engine_);
    for (size_t i=0; i<original_size; ++i) {
        const auto& p = cells[i];
        if (i >= num_resistant_cells) {
            p->set_cycle_dependent_death(*engine_, death_prob);
        }
    }
    const size_t margin = 10u * num_resistant_cells + 10u;
    grow(original_size + margin, std::numeric_limits<double>::max());
}

void Tissue::queue_push(const std::shared_ptr<Cell>& x, const bool surrounded) {
    double dt = x->delta_time(*engine_, time_, positional_value(x->coord()), surrounded);
    queue_.emplace_hint(queue_.end(), dt += time_, x);
}

void Tissue::init_insert_function(const std::string& local_density_effect, const std::string& displacement_path) {
    using func_t = std::function<bool(const std::shared_ptr<Cell>&)>;
    using map_sf = std::unordered_map<std::string, func_t>;
    std::unordered_map<std::string, map_sf> swtch;

    swtch["const"].emplace("random", [this](const std::shared_ptr<Cell>& daughter) {
        push(daughter, coord_func_->random_direction(*engine_));
        return true;
    });
    swtch["const"].emplace("mindrag", [this](const std::shared_ptr<Cell>& daughter) {
        push_minimum_drag(daughter);
        return true;
    });
    swtch["const"].emplace("minstraight", [this](const std::shared_ptr<Cell>& daughter) {
        push(daughter, to_nearest_empty(daughter->coord()));
        return true;
    });
    swtch["const"].emplace("roulette", [this](const std::shared_ptr<Cell>& daughter) {
        push(daughter, roulette_direction(daughter->coord()));
        return true;
    });
    swtch["const"].emplace("stroll", [this](const std::shared_ptr<Cell>& daughter) {
        stroll(daughter, coord_func_->random_direction(*engine_));
        return true;
    });
    swtch["step"].emplace("random", [this](const std::shared_ptr<Cell>& daughter) {
        if (num_empty_neighbors(daughter->coord()) == 0U) {return false;}
        push(daughter, coord_func_->random_direction(*engine_));
        return true;
    });
    swtch["step"].emplace("mindrag", [this](const std::shared_ptr<Cell>& daughter) {
        return insert_adjacent(daughter);
    });
    swtch["linear"].emplace("random", [this](const std::shared_ptr<Cell>& daughter) {
        const auto x = num_empty_neighbors(daughter->coord());
        if (x > 0U) {
            double prob = x;
            prob /= coord_func_->directions().size();
            if (wtl::generate_canonical(*engine_) < prob) {
                push(daughter, coord_func_->random_direction(*engine_));
                return true;
            }
        }
        return false;
    });
    swtch["linear"].emplace("mindrag", [this](const std::shared_ptr<Cell>& daughter) {
        daughter->add_coord(coord_func_->random_direction(*engine_));
        return extant_cells_.insert(daughter).second;
    });
    try {
        insert = swtch.at(local_density_effect).at(displacement_path);
    } catch (std::exception& e) {
        std::ostringstream oss;
        oss << "\n" << __FILE__ << ':' << __LINE__ << ':' << __PRETTY_FUNCTION__
            << "\nInvalid value for -L/-P ("
            << local_density_effect << "/" << displacement_path
            << "); choose from";
        for (const auto& p: swtch) {
            oss << "\n -L" << p.first << " -P " << wtl::keys(p.second);
        }
        throw std::runtime_error(oss.str());
    }
}

void Tissue::push(std::shared_ptr<Cell> moving, const coord_t& direction) {
    do {
        moving->add_coord(direction);
    } while (swap_existing(&moving));
}

void Tissue::push_minimum_drag(std::shared_ptr<Cell> moving) {
    do {
        moving->add_coord(to_nearest_empty(moving->coord()));
    } while (swap_existing(&moving));
}

void Tissue::stroll(std::shared_ptr<Cell> moving, const coord_t& direction) {
    while (!insert_adjacent(moving)) {
        moving->add_coord(direction);
        swap_existing(&moving);
    }
}

bool Tissue::insert_adjacent(const std::shared_ptr<Cell>& moving) {
    const auto present_coord = moving->coord();
    const auto& directions = coord_func_->directions();
    thread_local auto indices = wtl::seq_len<unsigned>(directions.size());
    std::shuffle(indices.begin(), indices.end(), *engine_);
    for (const auto i: indices) {
        moving->add_coord(directions[i]);
        if (extant_cells_.insert(moving).second) {
            return true;
        }
        moving->set_coord(present_coord);
    }
    return false;
}

bool Tissue::swap_existing(std::shared_ptr<Cell>* x) {
    auto result = extant_cells_.insert(*x);
    if (result.second) {
        return false;
    } else {
        std::shared_ptr<Cell> existing = std::move(*result.first);
        extant_cells_.insert(extant_cells_.erase(result.first), std::move(*x));
        *x = std::move(existing);
        return true;
    }
}

void Tissue::migrate(const std::shared_ptr<Cell>& migrant) {
    extant_cells_.erase(migrant);
    auto orig_pos = migrant->coord();
    migrant->add_coord(coord_func_->random_direction(*engine_));
    auto result = extant_cells_.insert(migrant);
    if (!result.second) {
        std::shared_ptr<Cell> existing = std::move(*result.first);
        extant_cells_.insert(extant_cells_.erase(result.first), migrant);
        existing->set_coord(std::move(orig_pos));
        extant_cells_.insert(std::move(existing));
    }
}

size_t Tissue::steps_to_empty(const coord_t& current, const coord_t& direction) const {
    thread_local const auto key = std::make_shared<Cell>();
    const auto& end = extant_cells_.end();
    key->set_coord(current);
    size_t steps = 0;
    do {
        key->add_coord(direction);
        ++steps;
    } while (extant_cells_.find(key) != end);
    return steps;
}

const coord_t& Tissue::to_nearest_empty(const coord_t& current) const {
    thread_local const auto key = std::make_shared<Cell>();
    const auto& end = extant_cells_.end();
    const auto& directions = coord_func_->directions();
    thread_local auto indices = wtl::seq_len<unsigned>(directions.size());
    std::shuffle(indices.begin(), indices.end(), *engine_);
    for (int radius = 1; true; ++radius) {
        for (const auto i: indices) {
            key->set_coord(current + directions[i] * radius);
            if (extant_cells_.find(key) == end) {
                return directions[i];
            }
        }
    }
}

coord_t Tissue::roulette_direction(const coord_t& current) const {
    auto directions = coord_func_->directions();
    std::shuffle(directions.begin(), directions.end(), *engine_);
    std::vector<double> roulette;
    for (const auto& d: directions) {
        const auto l = steps_to_empty(current, d);
        if (l == 0U) {return d;}
        roulette.push_back(1.0 / l);
    }
    return directions[wtl::roulette_select(roulette, *engine_)];
}

uint_fast8_t Tissue::num_empty_neighbors(const coord_t& coord) const {
    thread_local auto nb = std::make_shared<Cell>();
    const auto& end = extant_cells_.end();
    uint_fast8_t cnt = 0;
    for (const auto& d: coord_func_->directions()) {
        nb->set_coord(coord + d);
        if (extant_cells_.find(nb) == end) {++cnt;}
    }
    return cnt;
}

std::ostream& Tissue::write_history(std::ostream& ost) const {
    ost.precision(std::cout.precision());
    ost << Cell::header() << "\n";
    std::unordered_set<unsigned> done;
    for (const auto& p: extant_cells_) {
        p->traceback(ost, &done);
    }
    return ost;
}

std::ostream& Tissue::write_dead_history(std::ostream& ost) const {
    ost.precision(std::cout.precision());
    ost << Cell::header() << "\n";
    std::unordered_set<unsigned> done;
    for (const auto& p: dead_cells_) {
      p->traceback(ost, &done);
      //p->write(ost) << "\n";
    }
    return ost;
}

std::ostream& Tissue::write_snapshots(std::ostream& ost) const {
    ost << "time\t" << Cell::header() << "\n" << snapshots_.rdbuf();
    return ost;
}

// std::ostream& Tissue::write_drivers(std::ostream& ost) const {
//     ost << "id\ttype\tcoor\tcoef\n" << drivers_.rdbuf();
//     return ost;
// }

// Yunong Xia
std::ostream& Tissue::write_drivers(std::ostream& ost) const {
    ost << "cell_id\tprecursor_copy_id\tnew_copy_id\n" << drivers_.rdbuf();
    return ost;
}


std::ostream& Tissue::write_passengers(std::ostream& ost) const {
    ost << "id\tcopy_id\tsnv_id\n" << passengers_.rdbuf();
    return ost;
}

std::ostream& Tissue::write_seedingCells(std::ostream& ost) const {
    ost << "id\tNs\n" << seedingCells_.rdbuf();
    return ost;
}

std::ostream& Tissue::write_benchmark(std::ostream& ost) const {
    benchmark_->append(extant_cells_.size() + 1u);
    ost << benchmark_->rdbuf();
    return ost;
}

void Tissue::snapshots_append() {
    for (const auto& p: extant_cells_) {
        snapshots_ << time_ << "\t" << *p << "\n";
    }
}

//! Stream operator for debug print
std::ostream& operator<< (std::ostream& ost, const Tissue& tissue) {
    for (const auto& p: tissue.extant_cells_) {
        ost << *p << "\n";
    }
    return ost;
}

} // namespace tumopp
