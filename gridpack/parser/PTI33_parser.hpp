/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
/*
 * PTI33parser.hpp
 *
 *  Created on: December 10, 2014
 *      Author: Kevin Glass, Bruce Palmer
 */

#ifndef PTI33_PARSER_HPP_
#define PTI33_PARSER_HPP_

#define OLD_MAP

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp> // needed of is_any_of()
#ifndef OLD_MAP
#include <boost/unordered_map.hpp>
#endif
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>

#include "gridpack/utilities/exception.hpp"
#include "gridpack/timer/coarse_timer.hpp"
#include "gridpack/parser/dictionary.hpp"
#include "gridpack/network/base_network.hpp"
#include "gridpack/parser/base_parser.hpp"
#include "gridpack/parser/base_pti_parser.hpp"

#define TERM_CHAR '0'
// SOURCE: http://www.ee.washington.edu/research/pstca/formats/pti.txt

namespace gridpack {
namespace parser {


template <class _network>
class PTI33_parser : public BasePTIParser<_network>
{
  public:
    /// Constructor 
    /**
     * 
     * @param network network object that will be filled with contents
     * of network configuration file (must be child of network::BaseNetwork<>)
     */
    PTI33_parser(boost::shared_ptr<_network> network)
      : p_network(network), p_maxBusIndex(-1)
    {
      this->setNetwork(network);
    }

    /**
     * Destructor
     */
    virtual ~PTI33_parser()
    {
      p_busData.clear();      // unnecessary
      p_branchData.clear();
    }

    /**
     * Parse network configuration file and create network
     * @param fileName name of network file
     */
    void parse(const std::string &fileName)
    {
      p_timer = gridpack::utility::CoarseTimer::instance();
      p_timer->configTimer(false);
      int t_total = p_timer->createCategory("Parser:Total Elapsed Time");
      p_timer->start(t_total);
      std::string ext = this->getExtension(fileName);
      if (ext == "raw") {
        getCase(fileName);
        //brdcst_data();
        this->createNetwork(p_busData,p_branchData);
      } else if (ext == "dyr") {
        this->getDS(fileName);
      }
      p_timer->stop(t_total);
      p_timer->configTimer(true);
    }

  protected:
    /*
     * A case is the collection of all data associated with a PTI33 file.
     * Each case is a a vector of data_set objects the contain all the data
     * associated with a partition of the PTI file. For example, the bus
     * data in the file constitutes a data_set. Each data_set is a vector of
     * gridpack::component::DataCollection objects. Each of these objects
     * contain a single instance of the data associated with a data_set. For
     * example, each line of the bus partition corresponds to a single
     * DataCollection object.
     */
    void getCase(const std::string & fileName)
    {

      int t_case = p_timer->createCategory("Parser:getCase");
      p_timer->start(t_case);
      p_busData.clear();
      p_branchData.clear();
      p_busMap.clear();

      int me(p_network->communicator().rank());

      std::ifstream            input;
      if (me == 0) {
        input.open(fileName.c_str());
        if (!input.is_open()) {
          char buf[512];
          sprintf(buf,"Failed to open network configuration file: %s\n\n",
              fileName.c_str());
          throw gridpack::Exception(buf);
        }
        find_case(input);
      } else {
        p_case_sbase = 0.0;
        p_case_id = 0;
      }

      // Transmit CASE_SBASE to all processors
      double sval =  p_case_sbase;
      double rval;
      MPI_Comm comm = static_cast<MPI_Comm>(p_network->communicator());
      int nprocs = p_network->communicator().size();
      int ierr = MPI_Allreduce(&sval,&rval,1,MPI_DOUBLE,MPI_SUM,comm);
      p_case_sbase = rval;
      // Transmit CASE_ID to all processors
      int isval, irval;
      isval = p_case_id;
      ierr = MPI_Allreduce(&isval,&irval,1,MPI_INT,MPI_SUM,comm);
      p_case_id = irval;
      this->setCaseID(p_case_id);
      this->setCaseSBase(p_case_sbase);

      if (me == 0) {
        find_buses(input);
        find_loads(input);
        find_fixed_shunts(input);
        find_generators(input);
        find_branches(input);
        find_transformer(input);
#if 0
        find_area(input);
        find_2term(input);
        find_imped_corr(input);
        find_multi_term(input);
        find_multi_section(input);
        find_zone(input);
        find_interarea(input);
        find_owner(input);
        //find_line(input);
#endif
#if 0
        // debug
        int i;
        printf("BUS data size: %d\n",(int)p_busData.size());
        for (i=0; i<p_busData.size(); i++) {
          printf("Dumping bus: %d\n",i);
          p_busData[i]->dump();
        }
        printf("BRANCH data size: %d\n",(int)p_branchData.size());
        for (i=0; i<p_branchData.size(); i++) {
          printf("Dumping branch: %d\n",i);
          p_branchData[i]->dump();
        }
#endif
        input.close();
      }
      p_timer->stop(t_case);
    }

    void find_case(std::ifstream & input)
    {
      std::string                                        line;

      std::getline(input, line);
      while (check_comment(line)) {
        std::getline(input, line);
      }
      std::vector<std::string>  split_line;

      boost::algorithm::split(split_line, line, boost::algorithm::is_any_of(","),
          boost::token_compress_on);

      // CASE_ID             "IC"                   ranged integer
      p_case_id = atoi(split_line[0].c_str());

      // CASE_SBASE          "SBASE"                float
      p_case_sbase = atof(split_line[1].c_str());

      /*  These do not appear in the dictionary
      // REVISION_ID
      if (split_line.size() > 2)
      p_revision_id = atoi(split_line[2].c_str());

      // XFRRAT_UNITS
      if (split_line.size() > 3)
      p_xffrat_units = atof(split_line[3].c_str());

      // NXFRAT_UNITS
      if (split_line.size() > 4)
      p_nxfrat_units = atof(split_line[4].c_str());

      // BASE_FREQ
      if (split_line.size() > 5)
      p_base_freq = atof(split_line[5].c_str());
       */

    }

    void find_buses(std::ifstream & input)
    {
      std::string          line;
      int                  index = 0;
      int                  o_idx;
      std::getline(input, line);
      std::getline(input, line);
      std::getline(input, line);

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","),
            boost::token_compress_on);
        boost::shared_ptr<gridpack::component::DataCollection>
          data(new gridpack::component::DataCollection);
        int nstr = split_line.size();

        // BUS_I               "I"                   integer
        o_idx = atoi(split_line[0].c_str());
        if (p_maxBusIndex<o_idx) p_maxBusIndex = o_idx;
        data->addValue(BUS_NUMBER, o_idx);
        p_busData.push_back(data);
        p_busMap.insert(std::pair<int,int>(o_idx,index));

        // BUS_NAME             "NAME"                 string
        std::string bus_name = split_line[1];

        //store bus removes white space around name
        storeBus(bus_name, o_idx);
        if (nstr > 1) data->addValue(BUS_NAME, bus_name.c_str());

        // BUS_BASEKV           "BASKV"               float
        if (nstr > 2) data->addValue(BUS_BASEKV, atof(split_line[2].c_str()));

        // BUS_TYPE               "IDE"                   integer
        if (nstr > 3) data->addValue(BUS_TYPE, atoi(split_line[3].c_str()));

        // BUS_AREA            "IA"                integer
        if (nstr > 4) data->addValue(BUS_AREA, atoi(split_line[4].c_str()));

        // BUS_ZONE            "ZONE"                integer
        if (nstr > 5) data->addValue(BUS_ZONE, atoi(split_line[5].c_str()));

        // BUS_OWNER              "IA"                  integer
        if (nstr > 6) data->addValue(BUS_OWNER, atoi(split_line[6].c_str()));

        // BUS_VOLTAGE_MAG              "VM"                  float
        if (nstr > 7) data->addValue(BUS_VOLTAGE_MAG, atof(split_line[7].c_str()));

        // BUS_VOLTAGE_ANG              "VA"                  float
        if (nstr > 8) data->addValue(BUS_VOLTAGE_ANG, atof(split_line[8].c_str()));

        // BUS_VOLTAGE_MAX              "VOLTAGE_MAX"               float
        if (nstr > 9) data->addValue(BUS_VOLTAGE_MAX, atof(split_line[9].c_str()));

        // BUS_VOLTAGE_MIN              "VOLTAGE_MIN"              float
        if (nstr > 10) data->addValue(BUS_VOLTAGE_MIN, atof(split_line[10].c_str()));

        // TODO: Need to add EVHI, EVLO
        index++;
        std::getline(input, line);
      }
    }

    void find_loads(std::ifstream & input)
    {
      std::string          line;
      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","),
            boost::token_compress_on);

        // LOAD_BUSNUMBER               "I"                   integer
        int l_idx, o_idx;
        o_idx = getBusIndex(split_line[0]);
        if (o_idx < 0) {
          getline(input, line);
          continue;
        }
#ifdef OLD_MAP
        std::map<int, int>::iterator it;
#else
        boost::unordered_map<int, int>::iterator it;
#endif
        it = p_busMap.find(o_idx);
        if (it != p_busMap.end()) {
          l_idx = it->second;
        } else {
          std::getline(input, line);
          continue;
        }
        int nstr = split_line.size();
        // Find out how many loads are already on bus
        int nld;
        if (!p_busData[l_idx]->getValue(LOAD_NUMBER, &nld)) nld = 0;


        p_busData[l_idx]->addValue(LOAD_BUSNUMBER, o_idx, nld);

        gridpack::utility::StringUtils util;
        if (nstr > 1) {
          // Clean up 2 character tag
          std::string tag = util.clean2Char(split_line[1]);
          // LOAD_ID              "ID"                  integer
          p_busData[l_idx]->addValue(LOAD_ID, tag.c_str(), nld);
        }

        // LOAD_ID              "ID"                  integer
        std::string tag = util.clean2Char(split_line[1]);
        if (nstr > 1) p_busData[l_idx]->addValue(LOAD_ID, tag.c_str(),nld);

        // LOAD_STATUS              "ID"                  integer
        if (nstr > 2) p_busData[l_idx]->addValue(LOAD_STATUS,
            atoi(split_line[2].c_str()), nld);

        // LOAD_AREA            "AREA"                integer
        if (nstr > 3) p_busData[l_idx]->addValue(LOAD_AREA,
            atoi(split_line[3].c_str()), nld);

        // LOAD_ZONE            "ZONE"                integer
        if (nstr > 4) p_busData[l_idx]->addValue(LOAD_ZONE,
            atoi(split_line[4].c_str()), nld);

        // LOAD_PL              "PL"                  float
        if (nstr > 5) {
          if (nld == 0) p_busData[l_idx]->addValue(LOAD_PL, atof(split_line[5].c_str()));
          p_busData[l_idx]->addValue(LOAD_PL, atof(split_line[5].c_str()), nld);
        }

        // LOAD_QL              "QL"                  float
        if (nstr > 6) {
          if (nld == 0) p_busData[l_idx]->addValue(LOAD_QL, atof(split_line[6].c_str()));
          p_busData[l_idx]->addValue(LOAD_QL, atof(split_line[6].c_str()), nld);
        }

        // LOAD_IP              "IP"                  float
        if (nstr > 7) p_busData[l_idx]->addValue(LOAD_IP,
            atof(split_line[7].c_str()), nld);

        // LOAD_IQ              "IQ"                  float
        if (nstr > 8) p_busData[l_idx]->addValue(LOAD_IQ,
            atof(split_line[8].c_str()), nld);

        // LOAD_YP              "YP"                  float
        if (nstr > 9) p_busData[l_idx]->addValue(LOAD_YP,
            atof(split_line[9].c_str()), nld);

        // LOAD_YQ            "YQ"                integer
        if (nstr > 10) p_busData[l_idx]->addValue(LOAD_YQ,
            atoi(split_line[10].c_str()), nld);

        // TODO: add variables OWNER, SCALE, INTRPT

        // Increment number of loads in data object
        if (nld == 0) {
          nld = 1;
          p_busData[l_idx]->addValue(LOAD_NUMBER,nld);
        } else {
          nld++;
          p_busData[l_idx]->setValue(LOAD_NUMBER,nld);
        }

        std::getline(input, line);
      }
    }

    void find_fixed_shunts(std::ifstream & input)
    {
      std::string          line;
      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","),
            boost::token_compress_on);

        // SHUNT_BUSNUMBER               "I"                   integer
        int l_idx, o_idx;
        o_idx = getBusIndex(split_line[0]);
#ifdef OLD_MAP
        std::map<int, int>::iterator it;
#else
        boost::unordered_map<int, int>::iterator it;
#endif
        it = p_busMap.find(o_idx);
        if (it != p_busMap.end()) {
          l_idx = it->second;
        } else {
          std::getline(input, line);
          continue;
        }
        int nstr = split_line.size();

        // Find out how many shunts are already on bus
        int nshnt;
        if (!p_busData[l_idx]->getValue(SHUNT_NUMBER, &nshnt)) nshnt = 0;

        p_busData[l_idx]->addValue(SHUNT_BUSNUMBER, o_idx, nshnt);

        if (nstr > 1) {
          // Clean up 2 character tag
          gridpack::utility::StringUtils util;
          std::string tag = util.clean2Char(split_line[1]);
          // SHUNT_ID              "ID"                  integer
          p_busData[l_idx]->addValue(SHUNT_ID, tag.c_str(), nshnt);
        }

        // SHUNT_STATUS              "STATUS"                  integer
        if (nstr > 2) p_busData[l_idx]->addValue(SHUNT_STATUS,
            atoi(split_line[2].c_str()), nshnt);

        // BUS_SHUNT_GL              "GL"                  float
        if (nstr > 3) {
          if (nshnt==0) p_busData[l_idx]->addValue(BUS_SHUNT_GL,
              atof(split_line[3].c_str()));
          p_busData[l_idx]->addValue(BUS_SHUNT_GL,
              atof(split_line[3].c_str()),nshnt);
        }

        // BUS_SHUNT_BL              "BL"                  float
        if (nstr > 4) {
          if (nshnt == 0) p_busData[l_idx]->addValue(BUS_SHUNT_BL,
              atof(split_line[4].c_str()));
          p_busData[l_idx]->addValue(BUS_SHUNT_BL,
              atof(split_line[4].c_str()),nshnt);
        }

        // Increment number of shunts in data object
        if (nshnt == 0) {
          nshnt = 1;
          p_busData[l_idx]->addValue(SHUNT_NUMBER,nshnt);
        } else {
          nshnt++;
          p_busData[l_idx]->setValue(SHUNT_NUMBER,nshnt);
        }

        std::getline(input, line);
      }
    }

    void find_generators(std::ifstream & input)
    {
      std::string          line;
      std::getline(input, line); //this should be the first line of the block
      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","),
            boost::token_compress_on);

        // GENERATOR_BUSNUMBER               "I"                   integer
        int l_idx, o_idx;
        o_idx = getBusIndex(split_line[0]);
#ifdef OLD_MAP
        std::map<int, int>::iterator it;
#else
        boost::unordered_map<int, int>::iterator it;
#endif
        int nstr = split_line.size();
        it = p_busMap.find(o_idx);
        if (it != p_busMap.end()) {
          l_idx = it->second;
        } else {
          std::getline(input, line);
          continue;
        }

        // Find out how many generators are already on bus
        int ngen;
        if (!p_busData[l_idx]->getValue(GENERATOR_NUMBER, &ngen)) ngen = 0;


        p_busData[l_idx]->addValue(GENERATOR_BUSNUMBER, o_idx, ngen);

        // Clean up 2 character tag
        gridpack::utility::StringUtils util;
        std::string tag = util.clean2Char(split_line[1]);
        // GENERATOR_ID              "ID"                  integer
        p_busData[l_idx]->addValue(GENERATOR_ID, tag.c_str(), ngen);

        // GENERATOR_PG              "PG"                  float
        if (nstr > 2) p_busData[l_idx]->addValue(GENERATOR_PG, atof(split_line[2].c_str()),
            ngen);

        // GENERATOR_QG              "QG"                  float
        if (nstr > 3) p_busData[l_idx]->addValue(GENERATOR_QG, atof(split_line[3].c_str()),
            ngen);

        // GENERATOR_QMAX              "QT"                  float
        if (nstr > 4) p_busData[l_idx]->addValue(GENERATOR_QMAX,
            atof(split_line[4].c_str()), ngen);

        // GENERATOR_QMIN              "QB"                  float
        if (nstr > 5) p_busData[l_idx]->addValue(GENERATOR_QMIN,
            atof(split_line[5].c_str()), ngen);

        // GENERATOR_VS              "VS"                  float
        if (nstr > 6) p_busData[l_idx]->addValue(GENERATOR_VS, atof(split_line[6].c_str()),
            ngen);

        // GENERATOR_IREG            "IREG"                integer
        if (nstr > 7) p_busData[l_idx]->addValue(GENERATOR_IREG,
            atoi(split_line[7].c_str()), ngen);

        // GENERATOR_MBASE           "MBASE"               float
        if (nstr > 8) p_busData[l_idx]->addValue(GENERATOR_MBASE,
            atof(split_line[8].c_str()), ngen);

        // GENERATOR_ZSOURCE                                complex
        if (nstr > 10) p_busData[l_idx]->addValue(GENERATOR_ZSOURCE,
            gridpack::ComplexType(atof(split_line[9].c_str()),
              atof(split_line[10].c_str())), ngen);

        // GENERATOR_XTRAN                              complex
        if (nstr > 12) p_busData[l_idx]->addValue(GENERATOR_XTRAN,
            gridpack::ComplexType(atof(split_line[11].c_str()),
              atof(split_line[12].c_str())), ngen);

        // GENERATOR_GTAP              "GTAP"                  float
        if (nstr > 13) p_busData[l_idx]->addValue(GENERATOR_GTAP,
            atof(split_line[13].c_str()), ngen);

        // GENERATOR_STAT              "STAT"                  float
        if (nstr > 14)  p_busData[l_idx]->addValue(GENERATOR_STAT,
            atoi(split_line[14].c_str()), ngen);

        // GENERATOR_RMPCT           "RMPCT"               float
        if (nstr > 15) p_busData[l_idx]->addValue(GENERATOR_RMPCT,
            atof(split_line[15].c_str()), ngen);

        // GENERATOR_PMAX              "PT"                  float
        if (nstr > 16) p_busData[l_idx]->addValue(GENERATOR_PMAX,
            atof(split_line[16].c_str()), ngen);

        // GENERATOR_PMIN              "PB"                  float
        if (nstr > 17) p_busData[l_idx]->addValue(GENERATOR_PMIN,
            atof(split_line[17].c_str()), ngen);

        // TODO: add variables Oi, Fi, WMOD, WPF

        // Increment number of generators in data object
        if (ngen == 0) {
          ngen = 1;
          p_busData[l_idx]->addValue(GENERATOR_NUMBER,ngen);
        } else {
          ngen++;
          p_busData[l_idx]->setValue(GENERATOR_NUMBER,ngen);
        }

        std::getline(input, line);
      }
    }

    void find_branches(std::ifstream & input)
    {
      std::string line;
      int  o_idx1, o_idx2;
      int index = 0;

      std::getline(input, line); //this should be the first line of the block

      int nelems;
      while(test_end(line)) {
        std::pair<int, int> branch_pair;
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","),
            boost::token_compress_on);

        o_idx1 = getBusIndex(split_line[0]);
        o_idx2 = getBusIndex(split_line[1]);

        // Check to see if pair has already been created
        int l_idx = 0;
        branch_pair = std::pair<int,int>(o_idx1, o_idx2);
#ifdef OLD_MAP
        std::map<std::pair<int, int>, int>::iterator it;
#else
        boost::unordered_map<std::pair<int, int>, int>::iterator it;
#endif
        it = p_branchMap.find(branch_pair);

        bool switched = false;
        if (it != p_branchMap.end()) {
          l_idx = it->second;
          p_branchData[l_idx]->getValue(BRANCH_NUM_ELEMENTS,&nelems);
        } else {
          // Check to see if from and to buses have been switched
          std::pair<int, int> new_branch_pair;
          new_branch_pair = std::pair<int,int>(o_idx2, o_idx1);
          it = p_branchMap.find(new_branch_pair);
          if (it != p_branchMap.end()) {
//            printf("Found multiple lines with switched buses 1: %d 2: %d\n",
//                o_idx1,o_idx2);
            l_idx = it->second;
            p_branchData[l_idx]->getValue(BRANCH_NUM_ELEMENTS,&nelems);
            switched = true;
          } else {
            boost::shared_ptr<gridpack::component::DataCollection>
              data(new gridpack::component::DataCollection);
            l_idx = p_branchData.size();
            p_branchData.push_back(data);
            nelems = 0;
            p_branchData[l_idx]->addValue(BRANCH_NUM_ELEMENTS,nelems);
          }
        }

        if (nelems == 0) {
          // BRANCH_INDEX                                   integer
          p_branchData[l_idx]->addValue(BRANCH_INDEX, index);

          // BRANCH_FROMBUS            "I"                   integer
          p_branchData[l_idx]->addValue(BRANCH_FROMBUS, o_idx1);

          // BRANCH_TOBUS            "J"                   integer
          p_branchData[l_idx]->addValue(BRANCH_TOBUS, o_idx2);

          // add pair to branch map
          p_branchMap.insert(std::pair<std::pair<int, int>, int >(branch_pair,
                index));
          index++;
        }

        // BRANCH_SWITCHED
        p_branchData[l_idx]->addValue(BRANCH_SWITCHED, switched, nelems);

        int nstr = split_line.size();
        // Clean up 2 character tag
        gridpack::utility::StringUtils util;
        std::string tag = util.clean2Char(split_line[2]);
        // BRANCH_CKT          "CKT"                 character
        if (nstr > 2) p_branchData[l_idx]->addValue(BRANCH_CKT,
            tag.c_str(), nelems);

        // BRANCH_R            "R"                   float
        if (nstr > 3) p_branchData[l_idx]->addValue(BRANCH_R,
            atof(split_line[3].c_str()), nelems);

        // BRANCH_X            "X"                   float
        if (nstr > 4) p_branchData[l_idx]->addValue(BRANCH_X,
            atof(split_line[4].c_str()), nelems);

        // BRANCH_B            "B"                   float
        if (nstr > 5) p_branchData[l_idx]->addValue(BRANCH_B,
            atof(split_line[5].c_str()), nelems);

        // BRANCH_RATING_A        "RATEA"               float
        if (nstr > 6) p_branchData[l_idx]->addValue(BRANCH_RATING_A,
            atof(split_line[6].c_str()), nelems);

        // BBRANCH_RATING_        "RATEB"               float
        if (nstr > 7) p_branchData[l_idx]->addValue(BRANCH_RATING_B,
            atof(split_line[7].c_str()), nelems);

        // BRANCH_RATING_C        "RATEC"               float
        if (nstr > 8) p_branchData[l_idx]->addValue(BRANCH_RATING_C,
            atof(split_line[8].c_str()), nelems);

        // BRANCH_SHUNT_ADMTTNC_G1        "GI"               float
        if (nstr > 9) p_branchData[l_idx]->addValue(BRANCH_SHUNT_ADMTTNC_G1,
            atof(split_line[9].c_str()), nelems);

        // BRANCH_SHUNT_ADMTTNC_B1        "BI"               float
        if (nstr > 10) p_branchData[l_idx]->addValue(BRANCH_SHUNT_ADMTTNC_B1,
            atof(split_line[10].c_str()), nelems);

        // BRANCH_SHUNT_ADMTTNC_G2        "GJ"               float
        if (nstr > 11) p_branchData[l_idx]->addValue(BRANCH_SHUNT_ADMTTNC_G2,
            atof(split_line[11].c_str()), nelems);

        // BRANCH_SHUNT_ADMTTNC_B2        "BJ"               float
        if (nstr > 12) p_branchData[l_idx]->addValue(BRANCH_SHUNT_ADMTTNC_B2,
            atof(split_line[12].c_str()), nelems);

        // BRANCH_STATUS        "STATUS"               integer
        if (nstr > 13) p_branchData[l_idx]->addValue(BRANCH_STATUS,
            atoi(split_line[13].c_str()), nelems);

        // TODO: add variables LEN, Oi, Fi
        nelems++;
        p_branchData[l_idx]->setValue(BRANCH_NUM_ELEMENTS,nelems);
        std::getline(input, line);
      }
    }

    // Utility function to parse lines in 3-winding transformer block
    bool parse3WindXForm(std::vector<std::string>split_line, double *windv,
        double* ang, double *ratea, double *rateb, double *ratec)
    {
      *windv = atof(split_line[0].c_str());
      *ang = atof(split_line[2].c_str());
      *ratea = atof(split_line[3].c_str());
      *rateb = atof(split_line[4].c_str());
      *ratec = atof(split_line[5].c_str());
      bool ret = true;
      return ret && (split_line.size() > 5);
    }

    // This code is NOT handling these elements correctly. Need to bring
    // it in line with find_branch routine and the definitions in the
    // ex_pti_file
    void find_transformer(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      std::pair<int, int>   branch_pair;

      // Save current number of branches
      int index = p_branchData.size();

      bool wind3X = true;

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","),
            boost::token_compress_on);
        int o_idx1, o_idx2;
        o_idx1 = getBusIndex(split_line[0]);
        o_idx2 = getBusIndex(split_line[1]);

        // Check for 2-winding or 3-winding transformer
        int k = getBusIndex(split_line[2]);
        if (k != 0) {
          if (wind3X) {
            int o_idx3 = k;
            std::getline(input, line);
            std::vector<std::string>  split_line2;
            boost::split(split_line2, line, boost::algorithm::is_any_of(","),
                boost::token_compress_on);
            // Check to see if transformer is active
            int stat;
            stat = atoi(split_line[11].c_str());
            if (split_line2.size() < 4 || stat == 0) {
              std::getline(input, line);
              std::getline(input, line);
              std::getline(input, line);
              std::getline(input, line);
              continue;
            }
            // Get internal indices corresponding to buses 1,2,3
            int l_idx1, l_idx2, l_idx3;
            std::map<int,int>::iterator it;
            it = p_busMap.find(o_idx1);
            if (it != p_busMap.end()) {
              l_idx1 = it->second;
            } else {
              printf("No match found for bus %s\n",split_line[0].c_str());
            }
            it = p_busMap.find(o_idx2);
            if (it != p_busMap.end()) {
              l_idx2 = it->second;
            } else {
              printf("No match found for bus %s\n",split_line[1].c_str());
            }
            it = p_busMap.find(o_idx3);
            if (it != p_busMap.end()) {
              l_idx3 = it->second;
            } else {
              printf("No match found for bus %s\n",split_line[2].c_str());
            }
            // Create a new bus and three new branches. No need to check
            // previous branches to see if they match since they are all
            // linked to new bus. Also don't need to add these to the branch
            // map data structure since they cannot match any regular branch
            // lines or transformers
            boost::shared_ptr<gridpack::component::DataCollection>
              data(new gridpack::component::DataCollection);
            int n_idx = p_busData.size();
            p_busData.push_back(data);
            p_maxBusIndex++;
            data->addValue(BUS_NUMBER,p_maxBusIndex);
            char cbuf[128];
            sprintf(cbuf,"DUMMY_BUS-%d-%d-%d",o_idx1,o_idx2,o_idx3);
            data->addValue(BUS_NAME,cbuf);
            data->addValue(BUS_BASEKV,0.0);
            data->addValue(BUS_TYPE,1);
            int ival;
            p_busData[l_idx1]->getValue(BUS_AREA,&ival);
            data->addValue(BUS_AREA,ival);
            p_busData[l_idx1]->getValue(BUS_OWNER,&ival);
            data->addValue(BUS_OWNER, ival);
            double rval = 0.0;
            double rvol;
            p_busData[l_idx1]->getValue(BUS_VOLTAGE_MAG,&rvol);
            rval += rvol;
            p_busData[l_idx2]->getValue(BUS_VOLTAGE_MAG,&rvol);
            rval += rvol;
            p_busData[l_idx3]->getValue(BUS_VOLTAGE_MAG,&rvol);
            rval += rvol;
            rval = rval/3.0;
            rval = 1.0;
            data->addValue(BUS_VOLTAGE_MAG,rval);
            rval = 0.0;
            p_busData[l_idx1]->getValue(BUS_VOLTAGE_ANG,&rvol);
            rval += rvol;
            p_busData[l_idx2]->getValue(BUS_VOLTAGE_ANG,&rvol);
            rval += rvol;
            p_busData[l_idx3]->getValue(BUS_VOLTAGE_ANG,&rvol);
            rval += rvol;
            rval = rval/3.0;
            rval = 0.0;
            data->addValue(BUS_VOLTAGE_ANG,rval);

            // parse remainder of line 1
            double mag1, mag2;
            mag1 = atof(split_line[7].c_str());
            mag2 = atof(split_line[8].c_str());
            // Clean up 2 character tag
            gridpack::utility::StringUtils util;
            std::string tag = util.clean2Char(split_line[3]);

            // parse line 2
            double r12, r23, r31, x12, x23, x31, sb12, sb23, sb31;
            double r1, r2, r3, x1, x2, x3, b1, b2, b3;
            r12 = atof(split_line2[0].c_str());
            x12 = atof(split_line2[1].c_str());
            sb12 = atof(split_line2[2].c_str());
            r23 = atof(split_line2[3].c_str());
            x23 = atof(split_line2[4].c_str());
            sb23 = atof(split_line2[5].c_str());
            r31 = atof(split_line2[6].c_str());
            x31 = atof(split_line2[7].c_str());
            sb31 = atof(split_line2[8].c_str());
            r1 = 0.5*(r12+r31-r23);
            x1 = 0.5*(x12+x31-x23);
            b1 = 0.0;
            r2 = 0.5*(r12+r23-r31);
            x2 = 0.5*(x12+x23-x31);
            b2 = 0.0;
            r3 = 0.5*(r23+r31-r12);
            x3 = 0.5*(x23+x31-x12);
            b3 = 0.0;
            // correct R and X values, if necessary
            if (sb12 != p_case_sbase && sb12 == 0.0) {
              r1 = r1*p_case_sbase/sb12;
              x1 = x1*p_case_sbase/sb12;
            }
            if (sb23 != p_case_sbase && sb23 == 0.0) {
              r2 = r2*p_case_sbase/sb23;
              x2 = x2*p_case_sbase/sb23;
            }
            if (sb31 != p_case_sbase && sb31 == 0.0) {
              r3 = r3*p_case_sbase/sb31;
              x3 = x3*p_case_sbase/sb31;
            }

            // create branch between new bus and bus I
            int index = p_branchData.size();
            boost::shared_ptr<gridpack::component::DataCollection>
              data1(new gridpack::component::DataCollection);
            p_branchData.push_back(data1);
            std::getline(input, line);
            std::vector<std::string> split_line3;
            boost::split(split_line3, line, boost::algorithm::is_any_of(","),
                boost::token_compress_on);
            double windv, ang, ratea, rateb, ratec;
            parse3WindXForm(split_line3, &windv, &ang, &ratea, &rateb, &ratec);
            data1->addValue(BRANCH_INDEX,index);
            data1->addValue(BRANCH_FROMBUS,o_idx1);
            data1->addValue(BRANCH_TOBUS,p_maxBusIndex);
            data1->addValue(BRANCH_NUM_ELEMENTS,1);
            data1->addValue(BRANCH_CKT,tag.c_str(),0);
            data1->addValue(BRANCH_R,r1,0);
            data1->addValue(BRANCH_X,x1,0);
            data1->addValue(BRANCH_B,b1,0);
            data1->addValue(BRANCH_RATING_A,ratea,0);
            data1->addValue(BRANCH_RATING_B,rateb,0);
            data1->addValue(BRANCH_RATING_C,ratec,0);
            data1->addValue(BRANCH_TAP,windv,0);
            data1->addValue(BRANCH_SHIFT,ang,0);
            data1->addValue(BRANCH_SWITCHED,false,0);
            data1->addValue(BRANCH_SHUNT_ADMTTNC_G1,mag1,0);
            data1->addValue(BRANCH_SHUNT_ADMTTNC_B1,mag2,0);
            data1->addValue(BRANCH_SHUNT_ADMTTNC_G2,0.0,0);
            data1->addValue(BRANCH_SHUNT_ADMTTNC_B2,0.0,0);
            if (stat == 1 || stat == 2 || stat == 3) {
              data1->addValue(BRANCH_STATUS,1,0);
            } else {
              data1->addValue(BRANCH_STATUS,0,0);
            }

            // create branch between new bus and bus J
            index++;
            boost::shared_ptr<gridpack::component::DataCollection>
              data2(new gridpack::component::DataCollection);
            p_branchData.push_back(data2);
            std::getline(input, line);
            std::vector<std::string> split_line4;
            boost::split(split_line4, line, boost::algorithm::is_any_of(","),
                boost::token_compress_on);
            parse3WindXForm(split_line4, &windv, &ang, &ratea, &rateb, &ratec);
            data2->addValue(BRANCH_INDEX,index);
            data2->addValue(BRANCH_FROMBUS,o_idx2);
            data2->addValue(BRANCH_TOBUS,p_maxBusIndex);
            data2->addValue(BRANCH_NUM_ELEMENTS,1);
            data2->addValue(BRANCH_CKT,tag.c_str(),0);
            data2->addValue(BRANCH_R,r2,0);
            data2->addValue(BRANCH_X,x2,0);
            data2->addValue(BRANCH_B,b2,0);
            data2->addValue(BRANCH_RATING_A,ratea,0);
            data2->addValue(BRANCH_RATING_B,rateb,0);
            data2->addValue(BRANCH_RATING_C,ratec,0);
            data2->addValue(BRANCH_TAP,windv,0);
            data2->addValue(BRANCH_SHIFT,ang,0);
            data2->addValue(BRANCH_SWITCHED,false,0);
            data2->addValue(BRANCH_SHUNT_ADMTTNC_G1,0.0,0);
            data2->addValue(BRANCH_SHUNT_ADMTTNC_B1,0.0,0);
            data2->addValue(BRANCH_SHUNT_ADMTTNC_G2,0.0,0);
            data2->addValue(BRANCH_SHUNT_ADMTTNC_B2,0.0,0);
            if (stat == 1 || stat == 3 || stat == 4) {
              data2->addValue(BRANCH_STATUS,1,0);
            } else {
              data2->addValue(BRANCH_STATUS,0,0);
            }

            // create branch between new bus and bus K
            index++;
            boost::shared_ptr<gridpack::component::DataCollection>
              data3(new gridpack::component::DataCollection);
            p_branchData.push_back(data3);
            std::getline(input, line);
            std::vector<std::string> split_line5;
            boost::split(split_line5, line, boost::algorithm::is_any_of(","),
                boost::token_compress_on);
            parse3WindXForm(split_line5, &windv, &ang, &ratea, &rateb, &ratec);
            data3->addValue(BRANCH_INDEX,index);
            data3->addValue(BRANCH_FROMBUS,o_idx3);
            data3->addValue(BRANCH_TOBUS,p_maxBusIndex);
            data3->addValue(BRANCH_NUM_ELEMENTS,1);
            data3->addValue(BRANCH_CKT,tag.c_str(),0);
            data3->addValue(BRANCH_R,r3,0);
            data3->addValue(BRANCH_X,x3,0);
            data3->addValue(BRANCH_B,b3,0);
            data3->addValue(BRANCH_RATING_A,ratea,0);
            data3->addValue(BRANCH_RATING_B,rateb,0);
            data3->addValue(BRANCH_RATING_C,ratec,0);
            data3->addValue(BRANCH_TAP,windv,0);
            data3->addValue(BRANCH_SHIFT,ang,0);
            data3->addValue(BRANCH_SWITCHED,false,0);
            data3->addValue(BRANCH_SHUNT_ADMTTNC_G1,0.0,0);
            data3->addValue(BRANCH_SHUNT_ADMTTNC_B1,0.0,0);
            data3->addValue(BRANCH_SHUNT_ADMTTNC_G2,0.0,0);
            data3->addValue(BRANCH_SHUNT_ADMTTNC_B2,0.0,0);
            if (stat == 1 || stat == 2 || stat == 4) {
              data3->addValue(BRANCH_STATUS,1,0);
            } else {
              data3->addValue(BRANCH_STATUS,0,0);
            }
          } else {
            // Skip 3-winding transformers (for now)
            std::getline(input, line);
            std::getline(input, line);
            std::getline(input, line);
            std::getline(input, line);
            continue;
          }
        } else {
          std::getline(input, line);
          std::vector<std::string>  split_line2;
          boost::split(split_line2, line, boost::algorithm::is_any_of(","),
              boost::token_compress_on);

          std::getline(input, line);
          std::vector<std::string>  split_line3;
          boost::split(split_line3, line, boost::algorithm::is_any_of(","),
              boost::token_compress_on);
          // Skip last line of 2-winding transformer block
          std::getline(input, line);
          // find branch corresponding to this transformer line. If it doesn't
          // exist, create one
          int l_idx = 0;
          branch_pair = std::pair<int,int>(o_idx1, o_idx2);
#ifdef OLD_MAP
          std::map<std::pair<int, int>, int>::iterator it;
#else
          boost::unordered_map<std::pair<int, int>, int>::iterator it;
#endif
          it = p_branchMap.find(branch_pair);
          bool switched = false;
          int nelems;
          if (it != p_branchMap.end()) {
            l_idx = it->second;
            p_branchData[l_idx]->getValue(BRANCH_NUM_ELEMENTS,&nelems);
          } else {
            // Check to see if from and to buses have been switched
            std::pair<int, int> new_branch_pair;
            new_branch_pair = std::pair<int,int>(o_idx2, o_idx1);
            it = p_branchMap.find(new_branch_pair);
            if (it != p_branchMap.end()) {
              l_idx = it->second;
              p_branchData[l_idx]->getValue(BRANCH_NUM_ELEMENTS,&nelems);
              switched = true;
            } else {
              boost::shared_ptr<gridpack::component::DataCollection>
                data(new gridpack::component::DataCollection);
              l_idx = p_branchData.size();
              p_branchData.push_back(data);
              std::pair<std::pair<int,int>,int> item;
              item = std::pair<std::pair<int,int>,int>(branch_pair,l_idx);
              p_branchMap.insert(item);
              nelems = 0;
              p_branchData[l_idx]->addValue(BRANCH_NUM_ELEMENTS,nelems);
            }
          }

          // If nelems=0 then this is new branch. Add parameters defining
          // branch
          if (nelems == 0) {
            // BRANCH_INDEX                   integer
            p_branchData[l_idx]->addValue(BRANCH_INDEX,
                index);

            // BRANCH_FROMBUS            "I"  integer
            p_branchData[l_idx]->addValue(BRANCH_FROMBUS,
                o_idx1);

            // BRANCH_TOBUS              "J"  integer
            p_branchData[l_idx]->addValue(BRANCH_TOBUS,
                o_idx2);

            // add pair to branch map
            p_branchMap.insert(std::pair<std::pair<int,int>,int >
                (branch_pair, index));
            index++;
          }

          // Clean up 2 character tag
          gridpack::utility::StringUtils util;
          std::string tag = util.clean2Char(split_line[3]);
          // BRANCH_CKT          "CKT"                 character
          p_branchData[l_idx]->addValue(BRANCH_CKT, tag.c_str(), nelems);

          // Add remaining parameters from line 1
          /*
           * type: float
           * TRANSFORMER_MAG1
           */
          p_branchData[l_idx]->addValue(TRANSFORMER_MAG1,
              atof(split_line[7].c_str()),nelems);

          /*
           * type: float
           * TRANSFORMER_MAG2
           */
          p_branchData[l_idx]->addValue(TRANSFORMER_MAG2,
              atof(split_line[8].c_str()),nelems);

          /*
           * type: integer
           * BRANCH_STATUS
           */
          p_branchData[l_idx]->addValue(BRANCH_STATUS,
              atoi(split_line[11].c_str()),nelems);

          // Add parameters from line 2
          /*
           * type: float
           * SBASE2
           */
          double sbase2 = atof(split_line2[2].c_str());

          /*
           * type: float
           * BRANCH_R
           */
          double rval = atof(split_line2[0].c_str());
          if (sbase2 == p_case_sbase || sbase2 == 0.0) {
            p_branchData[l_idx]->addValue(BRANCH_R,rval,nelems);
          } else {
            rval = rval*p_case_sbase/sbase2;
            p_branchData[l_idx]->addValue(BRANCH_R,rval,nelems);
          }

          /*
           * type: float
           * BRANCH_X
           */
          rval = atof(split_line2[1].c_str());
          if (sbase2 == p_case_sbase || sbase2 == 0.0) {
            p_branchData[l_idx]->addValue(BRANCH_X,rval,nelems);
          } else {
            rval = rval*p_case_sbase/sbase2;
            p_branchData[l_idx]->addValue(BRANCH_X,rval,nelems);
          }

          /*
           * type: float
           * BRANCH_B
           */
          rval = 0.0;
          p_branchData[l_idx]->addValue(BRANCH_B,rval,nelems);

          // Add parameters from line 3
          /*
           * type: float
           * BRANCH_TAP
           */
          p_branchData[l_idx]->addValue(BRANCH_TAP,
              atof(split_line3[0].c_str()),nelems);

          /*
           * type: float
           * BRANCH_SHIFT
           */
          p_branchData[l_idx]->addValue(BRANCH_SHIFT,
              atof(split_line3[2].c_str()),nelems);

          /*
           * type: float
           * BRANCH_RATING_A
           */
          p_branchData[l_idx]->addValue(BRANCH_RATING_A,
              atof(split_line3[3].c_str()),nelems);

          /*
           * type: float
           * BRANCH_RATING_B
           */
          p_branchData[l_idx]->addValue(BRANCH_RATING_B,
              atof(split_line3[4].c_str()),nelems);

          /*
           * type: float
           * BRANCH_RATING_C
           */
          p_branchData[l_idx]->addValue(BRANCH_RATING_C,
              atof(split_line3[4].c_str()),nelems);

          nelems++;
          p_branchData[l_idx]->setValue(BRANCH_NUM_ELEMENTS,nelems);
        }
        std::getline(input, line);
      }
    }

#if 0
    void find_area(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","),
            boost::token_compress_on);

        // AREAINTG_ISW           "ISW"                  integer
        int l_idx, o_idx;
        o_idx = atoi(split_line[1].c_str());
#ifdef OLD_MAP
        std::map<int, int>::iterator it;
#else
        boost::unordered_map<int, int>::iterator it;
#endif
        it = p_busMap.find(o_idx);
        if (it != p_busMap.end()) {
          l_idx = it->second;
        } else {
          std::getline(input, line);
          continue;
        }
        p_busData[l_idx]->addValue(AREAINTG_ISW, atoi(split_line[1].c_str()));

        // AREAINTG_NUMBER             "I"                    integer
        p_busData[l_idx]->addValue(AREAINTG_NUMBER, atoi(split_line[0].c_str()));

        // AREAINTG_PDES          "PDES"                 float
        p_busData[l_idx]->addValue(AREAINTG_PDES, atof(split_line[2].c_str()));

        // AREAINTG_PTOL          "PTOL"                 float
        p_busData[l_idx]->addValue(AREAINTG_PTOL, atof(split_line[3].c_str()));

        // AREAINTG_NAME         "ARNAM"                string
        p_busData[l_idx]->addValue(AREAINTG_NAME, split_line[4].c_str());

        std::getline(input, line);
      }
    }

    void find_2term(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","), boost::token_compress_on);
        std::getline(input, line);
      }
    }

    void find_line(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","), boost::token_compress_on);
        std::getline(input, line);
      }
    }


    /*

     */
    void find_shunt(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block
      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","), boost::token_compress_on);

        /*
         * type: integer
         * #define SHUNT_BUSNUMBER "SHUNT_BUSNUMBER"
         */
        int l_idx, o_idx;
        l_idx = atoi(split_line[0].c_str());
#ifdef OLD_MAP
        std::map<int, int>::iterator it;
#else
        boost::unordered_map<int, int>::iterator it;
#endif
        it = p_busMap.find(l_idx);
        if (it != p_busMap.end()) {
          o_idx = it->second;
        } else {
          std::getline(input, line);
          continue;
        }
        int nval = split_line.size();

        p_busData[o_idx]->addValue(SHUNT_BUSNUMBER, atoi(split_line[0].c_str()));

        /*
         * type: integer
         * #define SHUNT_MODSW "SHUNT_MODSW"
         */
        p_busData[o_idx]->addValue(SHUNT_MODSW, atoi(split_line[1].c_str()));

        /*
         * type: real float
         * #define SHUNT_VSWHI "SHUNT_VSWHI"
         */
        p_busData[o_idx]->addValue(SHUNT_VSWHI, atof(split_line[2].c_str()));

        /*
         * type: real float
         * #define SHUNT_VSWLO "SHUNT_VSWLO"
         */
        p_busData[o_idx]->addValue(SHUNT_VSWLO, atof(split_line[3].c_str()));

        /*
         * type: integer
         * #define SHUNT_SWREM "SHUNT_SWREM"
         */
        p_busData[o_idx]->addValue(SHUNT_SWREM, atoi(split_line[4].c_str()));

        /*
         * type: real float
         * #define SHUNT_RMPCT "SHUNT_RMPCT"
         */
        //          p_busData[o_idx]->addValue(SHUNT_RMPCT, atof(split_line[4].c_str()));

        /*
         * type: string
         * #define SHUNT_RMIDNT "SHUNT_RMIDNT"
         */
        //          p_busData[o_idx]->addValue(SHUNT_RMIDNT, split_line[5].c_str());

        /*
         * type: real float
         * #define SHUNT_BINIT "SHUNT_BINIT"
         */
        p_busData[o_idx]->addValue(SHUNT_BINIT, atof(split_line[5].c_str()));

        /*
         * type: integer
         * #define SHUNT_N1 "SHUNT_N1"
         */
        p_busData[o_idx]->addValue(SHUNT_N1, atoi(split_line[6].c_str()));

        /*
         * type: integer
         * #define SHUNT_N2 "SHUNT_N2"
         */
        if (8<nval) 
          p_busData[o_idx]->addValue(SHUNT_N2, atoi(split_line[8].c_str()));

        /*
         * type: integer
         * #define SHUNT_N3 "SHUNT_N3"
         */
        if (10<nval) 
          p_busData[o_idx]->addValue(SHUNT_N3, atoi(split_line[10].c_str()));

        /*
         * type: integer
         * #define SHUNT_N4 "SHUNT_N4"
         */
        if (12<nval) 
          p_busData[o_idx]->addValue(SHUNT_N4, atoi(split_line[12].c_str()));

        /*
         * type: integer
         * #define SHUNT_N5 "SHUNT_N5"
         */
        if (14<nval) 
          p_busData[o_idx]->addValue(SHUNT_N5, atoi(split_line[14].c_str()));

        /*
         * type: integer
         * #define SHUNT_N6 "SHUNT_N6"
         */
        if (16<nval) 
          p_busData[o_idx]->addValue(SHUNT_N6, atoi(split_line[16].c_str()));

        /*
         * type: integer
         * #define SHUNT_N7 "SHUNT_N7"
         */
        if (18<nval) 
          p_busData[o_idx]->addValue(SHUNT_N7, atoi(split_line[18].c_str()));

        /*
         * type: integer
         * #define SHUNT_N8 "SHUNT_N8"
         */
        if (20<nval) 
          p_busData[o_idx]->addValue(SHUNT_N8, atoi(split_line[20].c_str()));

        /*
         * type: real float
         * #define SHUNT_B1 "SHUNT_B1"
         */
        if (7<nval) 
          p_busData[o_idx]->addValue(SHUNT_B1, atof(split_line[7].c_str()));

        /*
         * type: real float
         * #define SHUNT_B2 "SHUNT_B2"
         */
        if (9<nval) 
          p_busData[o_idx]->addValue(SHUNT_B2, atof(split_line[9].c_str()));

        /*
         * type: real float
         * #define SHUNT_B3 "SHUNT_B3"
         */
        if (11<nval) 
          p_busData[o_idx]->addValue(SHUNT_B3, atof(split_line[11].c_str()));

        /*
         * type: real float
         * #define SHUNT_B4 "SHUNT_B4"
         */
        if (13<nval) 
          p_busData[o_idx]->addValue(SHUNT_B4, atof(split_line[13].c_str()));

        /*
         * type: real float
         * #define SHUNT_B5 "SHUNT_B5"
         */
        if (15<nval) 
          p_busData[o_idx]->addValue(SHUNT_B5, atof(split_line[15].c_str()));

        /*
         * type: real float
         * #define SHUNT_B6 "SHUNT_B6"
         */
        if (17<nval) 
          p_busData[o_idx]->addValue(SHUNT_B6, atof(split_line[17].c_str()));

        /*
         * type: real float
         * #define SHUNT_B7 "SHUNT_B7"
         */
        if (19<nval) 
          p_busData[o_idx]->addValue(SHUNT_B7, atof(split_line[19].c_str()));

        /*
         * type: real float
         * #define SHUNT_B8 "SHUNT_B8"
         */
        if (21<nval) 
          p_busData[o_idx]->addValue(SHUNT_B8, atof(split_line[21].c_str()));

        std::getline(input, line);
      }
    }

    void find_imped_corr(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","), boost::token_compress_on);
#if 0
        std::vector<gridpack::component::DataCollection>   imped_corr_instance;
        gridpack::component::DataCollection          data;

        /*
         * type: integer
         * #define XFMR_CORR_TABLE_NUMBER "XFMR_CORR_TABLE_NUMBER"
         */
        data.addValue(XFMR_CORR_TABLE_NUMBER, atoi(split_line[0].c_str()));
        imped_corr_instance.push_back(data);

        /*
         * type: real float
         * #define XFMR_CORR_TABLE_Ti "XFMR_CORR_TABLE_Ti"
         */
        data.addValue(XFMR_CORR_TABLE_Ti, atoi(split_line[0].c_str()));
        imped_corr_instance.push_back(data);

        /*
         * type: real float
         * #define XFMR_CORR_TABLE_Fi "XFMR_CORR_TABLE_Fi"
         */
        data.addValue(XFMR_CORR_TABLE_Fi, atoi(split_line[0].c_str()));
        imped_corr_instance.push_back(data);

        imped_corr_set.push_back(imped_corr_instance);
#endif
        std::getline(input, line);
      }
    }

    void find_multi_section(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
#if 0
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","), boost::token_compress_on);
        std::vector<gridpack::component::DataCollection>   multi_section_instance;
        gridpack::component::DataCollection          data;

        /*
         * type: integer
         * #define MULTI_SEC_LINE_FROMBUS "MULTI_SEC_LINE_FROMBUS"

         */
        data.addValue(MULTI_SEC_LINE_FROMBUS, atoi(split_line[0].c_str()));
        multi_section_instance.push_back(data);

        /*
         * type: integer
         * #define MULTI_SEC_LINE_TOBUS "MULTI_SEC_LINE_TOBUS"

         */
        data.addValue(MULTI_SEC_LINE_TOBUS, atoi(split_line[0].c_str()));
        multi_section_instance.push_back(data);

        /*
         * type: string
         * #define MULTI_SEC_LINE_ID "MULTI_SEC_LINE_ID"

         */
        data.addValue(MULTI_SEC_LINE_ID, split_line[0].c_str());
        multi_section_instance.push_back(data);

        /*
         * type: integer
         * #define MULTI_SEC_LINE_DUMi "MULTI_SEC_LINE_DUMi"
         */
        data.addValue(MULTI_SEC_LINE_DUMi, atoi(split_line[0].c_str()));
        multi_section_instance.push_back(data);

        multi_section.push_back(multi_section_instance);
#endif
        std::getline(input, line);
      }
    }

    void find_multi_term(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
        // TODO: parse something here
        std::getline(input, line);
      }
    }
    /*
     * ZONE_I          "I"                       integer
     * ZONE_NAME       "NAME"                    string
     */
    void find_zone(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block
      while(test_end(line)) {
        // TODO: parse something here
        std::getline(input, line);
      }
    }

    void find_interarea(std::ifstream & input)
    {
      std::string          line;

      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
#if 0
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","), boost::token_compress_on);
        std::vector<gridpack::component::DataCollection>   inter_area_instance;
        gridpack::component::DataCollection          data;

        /*
         * type: integer
         * #define INTERAREA_TRANSFER_FROM "INTERAREA_TRANSFER_FROM"
         */
        data.addValue(INTERAREA_TRANSFER_FROM, atoi(split_line[0].c_str()));
        inter_area_instance.push_back(data);

        /*
         * type: integer
         * #define INTERAREA_TRANSFER_TO "INTERAREA_TRANSFER_TO"
         */
        data.addValue(INTERAREA_TRANSFER_TO, atoi(split_line[0].c_str()));
        inter_area_instance.push_back(data);

        /*
         * type: character
         * #define INTERAREA_TRANSFER_TRID "INTERAREA_TRANSFER_TRID"
         */
        data.addValue(INTERAREA_TRANSFER_TRID, split_line[0].c_str()[0]);
        inter_area_instance.push_back(data);

        /*
         * type: real float
         * #define INTERAREA_TRANSFER_PTRAN "INTERAREA_TRANSFER_PTRAN"
         */
        data.addValue(INTERAREA_TRANSFER_PTRAN, atof(split_line[0].c_str()));
        inter_area_instance.push_back(data);

        inter_area.push_back(inter_area_instance);
#endif
        std::getline(input, line);
      }
    }

    /*
     * type: integer
     * #define OWNER_NUMBER "OWNER_NUMBER"

     * type: integer
     * #define OWNER_NAME "OWNER_NAME"
     */
    void find_owner(std::ifstream & input)
    {
      std::string          line;
      std::getline(input, line); //this should be the first line of the block

      while(test_end(line)) {
#if 0
        std::vector<std::string>  split_line;
        boost::split(split_line, line, boost::algorithm::is_any_of(","), boost::token_compress_on);
        std::vector<gridpack::component::DataCollection>   owner_instance;
        gridpack::component::DataCollection          data;

        data.addValue(OWNER_NUMBER, atoi(split_line[0].c_str()));
        owner_instance.push_back(data);

        data.addValue(OWNER_NAME, split_line[1].c_str());
        owner_instance.push_back(data);

        owner.push_back(owner_instance);
#endif
        std::getline(input, line);
      }
    }
#endif

    // Distribute data uniformly on processors
    void brdcst_data(void)
    {
      int t_brdcst = p_timer->createCategory("Parser:brdcst_data");
      int t_serial = p_timer->createCategory("Parser:data packing and unpacking");
      MPI_Comm comm = static_cast<MPI_Comm>(p_network->communicator());
      int me(p_network->communicator().rank());
      int nprocs(p_network->communicator().size());
      if (nprocs == 1) return;
      p_timer->start(t_brdcst);

      // find number of buses and branches and broadcast this information to
      // all processors
      int sbus, sbranch;
      if (me == 0) {
        sbus = p_busData.size();
        sbranch = p_branchData.size();
      } else {
        sbus = 0;
        sbranch = 0;
      }
      int ierr, nbus, nbranch;
      ierr = MPI_Allreduce(&sbus, &nbus, 1, MPI_INT, MPI_SUM, comm);
      ierr = MPI_Allreduce(&sbranch, &nbranch, 1, MPI_INT, MPI_SUM, comm);
      double rprocs = static_cast<double>(nprocs);
      double rme = static_cast<double>(me);
      int n, i;
      std::vector<gridpack::component::DataCollection>recvV;
      // distribute buses
      if (me == 0) {
        for (n=0; n<nprocs; n++) {
          double rn = static_cast<double>(n);
          int istart = static_cast<int>(static_cast<double>(nbus)*rn/rprocs);
          int iend = static_cast<int>(static_cast<double>(nbus)*(rn+1.0)/rprocs);
          if (n != 0) {
            p_timer->start(t_serial);
            std::vector<gridpack::component::DataCollection> sendV;
            for (i=istart; i<iend; i++) {
              sendV.push_back(*(p_busData[i]));
            }
            p_timer->stop(t_serial);
            static_cast<boost::mpi::communicator>(p_network->communicator()).send(n,n,sendV);
          } else {
            p_timer->start(t_serial);
            for (i=istart; i<iend; i++) {
              recvV.push_back(*(p_busData[i]));
            }
            p_timer->stop(t_serial);
          }
        }
      } else {
        int istart = static_cast<int>(static_cast<double>(nbus)*rme/rprocs);
        int iend = static_cast<int>(static_cast<double>(nbus)*(rme+1.0)/rprocs)-1;
        static_cast<boost::mpi::communicator>(p_network->communicator()).recv(0,me,recvV);
      }
      int nsize = recvV.size();
      p_busData.clear();
      p_timer->start(t_serial);
      for (i=0; i<nsize; i++) {
        boost::shared_ptr<gridpack::component::DataCollection> data(new
            gridpack::component::DataCollection);
        *data = recvV[i];
        p_busData.push_back(data);
      }
      p_timer->stop(t_serial);
      recvV.clear();
      // distribute branches
      if (me == 0) {
        for (n=0; n<nprocs; n++) {
          double rn = static_cast<double>(n);
          int istart = static_cast<int>(static_cast<double>(nbranch)*rn/rprocs);
          int iend = static_cast<int>(static_cast<double>(nbranch)*(rn+1.0)/rprocs);
          if (n != 0) {
            p_timer->start(t_serial);
            std::vector<gridpack::component::DataCollection> sendV;
            for (i=istart; i<iend; i++) {
              sendV.push_back(*(p_branchData[i]));
            }
            p_timer->stop(t_serial);
            static_cast<boost::mpi::communicator>(p_network->communicator()).send(n,n,sendV);
          } else {
            p_timer->start(t_serial);
            for (i=istart; i<iend; i++) {
              recvV.push_back(*(p_branchData[i]));
            }
            p_timer->stop(t_serial);
          }
        }
      } else {
        int istart = static_cast<int>(static_cast<double>(nbranch)*rme/rprocs);
        int iend = static_cast<int>(static_cast<double>(nbranch)*(rme+1.0)/rprocs)-1;
        static_cast<boost::mpi::communicator>(p_network->communicator()).recv(0,me,recvV);
      }
      nsize = recvV.size();
      p_branchData.clear();
      p_timer->start(t_serial);
      for (i=0; i<nsize; i++) {
        boost::shared_ptr<gridpack::component::DataCollection> data(new
            gridpack::component::DataCollection);
        *data = recvV[i];
        p_branchData.push_back(data);
      }
      p_timer->stop(t_serial);
#if 0
      // debug
      printf("p[%d] BUS data size: %d\n",me,p_busData.size());
      for (i=0; i<p_busData.size(); i++) {
        printf("p[%d] Dumping bus: %d\n",me,i);
        p_busData[i]->dump();
      }
      printf("p[%d] BRANCH data size: %d\n",me,p_branchData.size());
      for (i=0; i<p_branchData.size(); i++) {
        printf("p[%d] Dumping branch: %d\n",me,i);
        p_branchData[i]->dump();
      }
#endif
      p_timer->stop(t_brdcst);
    }

  private:
    /**
     * Test to see if string terminates a section
     * @return: false if first non-blank character is TERM_CHAR
     */
    bool test_end(std::string &str) const
    {
#if 1
      if (str[0] == TERM_CHAR) {
        return false;
      }
      int len = str.length();
      int i=0;
      while (i<len && str[i] == ' ') {
        i++;
      }
      if (i<len && str[i] != TERM_CHAR) {
        return true;
      } else if (i == len) {
        return true;
      } else if (str[i] == TERM_CHAR) {
        i++;
        if (i>=len || str[i] == ' ' || str[i] == '\\') {
          return false;
        } else {
          return true;
        }
      } else {
        return true;
      }
#else
      if (str[0] == '0') {
        return false;
      } else {
        return true;
      }
#endif
    }

    /**
     * Test to see if string is a comment line. Check to see if first
     * non-blank characters are "//"
     */
    bool check_comment(std::string &str) const
    {
      int ntok = str.find_first_not_of(' ',0);
      if (ntok != std::string::npos && ntok+1 != std::string::npos &&
          str[ntok] == '/' && str[ntok+1] == '/') {
        return true;
      } else {
        return false;
      }
    }

    /**
     * Remove leading and trailing blanks
     */
    void remove_blanks(std::string &str)
    {
      int ntok1 = str.find_first_not_of(' ',0);
      int ntok2 = str.length()-1;
      while (str[ntok2] == ' ') {ntok2--;}
      str = str.substr(ntok1, ntok2-ntok1+1);
    }

    /**
     * Check to see if string represents a character string. Look for single
     * quotes. Strip off white space and remove quotes if returning true
     */
    bool check_string(std::string &str)
    {
      int ntok1 = str.find_first_not_of(' ',0);
      if (ntok1 != std::string::npos && str[ntok1] == '\'') {
        ntok1++;
        int ntok2 = str.find_first_of('\'',ntok1);
        if (ntok2-ntok1 > 1 && ntok2 != std::string::npos) {
          ntok2--;
          str = str.substr(ntok1, ntok2-ntok1+1);
        } else if (ntok2 == std::string::npos && str.length()-1 - ntok1 > 1) {
          ntok2 = str.length()-1;
          str = str.substr(ntok1, ntok2-ntok1+1);
        } else {
          str.clear();
        }
        // strip off any remaining trailing blanks
        if (str.length() > 0) remove_blanks(str);
        return true;
      } else {
        // string may not have single quotes but remove leading and trailing
        // blanks, if any
        remove_blanks(str);
        return false;
      }
    }

    /**
     * Split bus name into its name and base bus voltage. Assume that incoming
     * string has been stripped of single quotes and leading and trailing
     * white space. The name and the voltage are suppose to be delimited by at
     * least 3 white spaces
     */
    void parseBusName(std::string &string, std::string &name, double &voltage)
    {
      // If string contains 12 or less characters, assume it is just a name
      name = string;
      voltage = 0.0;
      int len = string.length();
      if (len > 12) {
        // look for 3 consecutive blank characters
        int ntok1 = string.find("   ",0);
        if (ntok1 != std::string::npos) {
          name = string.substr(0,ntok1);
          int ntok2 = string.find_first_not_of(' ',ntok1);
          if (ntok2 != std::string::npos) {
            voltage = atof(string.substr(ntok2,len-ntok2).c_str());
          }
        }
      }
      return;
    }

    /**
     * add bus name to p_nameMap data structure, along with the original bus
     * index. Assume bus name is not paired with base bus voltage.
     */
    void storeBus(std::string &string, int idx)
    {
      check_string(string);
      std::pair<std::string,int> name_pair;
      name_pair = std::pair<std::string,int>(string,abs(idx));
      p_nameMap.insert(name_pair);
    }

    /**
     * Get bus index from bus name string. If the bus name string does not
     * have quotes, assume it represents an integer index. If it does have
     * quotes, find the corresponding index in the p_nameMap data structure
     */
    int getBusIndex(std::string str)
    {
      if (check_string(str)) {
        std::string name;
        double voltage;
        parseBusName(str,name,voltage);
#ifdef OLD_MAP
        std::map<std::string,int>::iterator it;
#else
        boost::unordered_map<std::string, int>::iterator it;
#endif
        it = p_nameMap.find(name);
        if (it != p_nameMap.end()) {
          return it->second;
        } else {
          return -1;
        }
      } else {
        return abs(atoi(str.c_str()));
      }
    }

    /*
     * The case_data is the collection of all data points in the case file.
     * Each collection in the case data contains the data associated with a given
     * type. For example, the case is the collection of data describing the
     * current case and the bus data is the collection of data associated with
     * each bus. The type data may consist of zero or more instances of the
     * given type. For example, the bus data may contain several instances of
     * a bus. These type instances are composed of a set of key value pairs.
     * Each column as an associated key and each row is an instance of a given
     * type. When the parser is reading data for a type, the value found in each
     * column associated with the key for that column in a field_data structure.
     *
     * Within the PTI file there are the following group of data sets in order:
     *     case
     *     bus
     *     generator
     *     branch
     *     transformer
     *     dc_line
     *     shunt
     *     impedence corr
     *     multi-terminal
     *     multi-section
     *     zone
     *     inter-area
     *     owner
     *     device driver
     *
     * These data sets are stored in the case data as a collection of
     * data set and each data set is a
     */
    boost::shared_ptr<_network> p_network;

    // Vector of bus data objects
    std::vector<boost::shared_ptr<gridpack::component::DataCollection> > p_busData;
    // Vector of branch data objects
    std::vector<boost::shared_ptr<gridpack::component::DataCollection> > p_branchData;
    // Map of PTI indices to index in p_busData
#ifdef OLD_MAP
    std::map<int,int> p_busMap;
    std::map<std::string,int> p_nameMap;
#else
    boost::unordered_map<int, int> p_busMap;
    boost::unordered_map<std::string, int> p_nameMap;
#endif
    // Map of PTI index pair to index in p_branchData
#ifdef OLD_MAP
    std::map<std::pair<int, int>, int> p_branchMap;
#else
    boost::unordered_map<std::pair<int, int>, int> p_branchMap;
#endif

    // Global variables that apply to whole network
    int p_case_id;
    int p_maxBusIndex;
    double p_case_sbase;
    gridpack::utility::CoarseTimer *p_timer;
};

} /* namespace parser */
} /* namespace gridpack */
#endif /* PTI33PARSER_HPP_ */
