/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>
#include "ns3/core-module.h"
#include <sys/types.h>
#include <sys/stat.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ScratchSimulator");

int mkpath(std::string s,mode_t mode=0755)
{
    size_t pre=0,pos;
    std::string dir;
    int mdret = 0;

    if(s[s.size()-1]!='/'){
        // force trailing / so we can handle everything in loop
        s+='/';
    }

    while((pos=s.find_first_of('/',pre))!=std::string::npos){
        dir=s.substr(0,pos++);
        pre=pos;
        if(dir.size()==0) continue; // if leading / first time is 0 length
        if((mdret=::mkdir(dir.c_str(),mode)) && errno!=EEXIST){
            return mdret;
        }
    }
    return mdret;
}

int 
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  std::string haha = "erwer/ytryew/564jhf/gh";
  std::cout << haha.size() << std::endl;
  std::cout << haha.find_last_of ("/") << std::endl;
  std::cout << (haha.substr(haha.find_last_of ("/") + 1, haha.size())) << std::endl;

}
