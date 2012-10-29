/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <iostream>
#include <boost/program_options.hpp>
#include "serialstream.h"

using namespace std;
using namespace boost::posix_time;
using namespace boost::program_options;

int main(int argc, char* argv[])
{
    options_description desc;
    desc.add_options()
        ("help","Prints this")
        ("arduino",value<string>(),"Serial port to which the arduino is attached")
        ("event","Genearte an event")
        ("device",value<int>(),"Select which device to operate on 0..2")
        ("enter","Enter firmware programming")
        ("leave","Leave firmware programming (or reset the board)")
        ("keep","Keep the device in reset state");
    ;
    variables_map vm;
    store(parse_command_line(argc,argv,desc),vm);
    notify(vm);
    if(vm.count("help")) { cout<<desc<<endl; return 0; }
    if(!vm.count("arduino")) throw(runtime_error("No arduino port is given"));
    if((vm.count("device") ^ vm.count("event"))==0)
        throw runtime_error("Either specify a device or 'event'");
    if(vm.count("enter") && vm.count("leave"))
        throw runtime_error("enter and leave are mutually exclusive");
    SerialOptions options;
    options.setDevice(vm["arduino"].as<string>());
    options.setBaudrate(19200);
    options.setTimeout(seconds(3));
    SerialStream serial(options);
    serial.exceptions(ios::badbit | ios::failbit); //Important!
    if(vm.count("event"))
    {
        serial<<"e\r";
        serial.flush();
        for(int i=0;i<3;i++)
        {
            string line;
            getline(serial,line);
            if(!line.empty() && line[line.length()-1]=='\r') line=line.substr(0,line.length()-1);
            if(line=="event") return 0;
        }
        throw runtime_error("event not generated");
    }
    if(vm.count("enter"))
    {
        stringstream ss;
        ss<<vm["device"].as<int>()<<" "<<"e";
        serial<<ss.str()<<"\r";
        serial.flush();
        for(int i=0;i<3;i++)
        {
            string line;
            getline(serial,line);
            if(!line.empty() && line[line.length()-1]=='\r') line=line.substr(0,line.length()-1);
            if(line==ss.str()) return 0;
        }
        throw runtime_error("failed entering firmware upgrade");
    }
    if(vm.count("leave"))
    {
        stringstream ss;
        ss<<vm["device"].as<int>()<<" "<<"l";
        serial<<ss.str()<<"\r";
        serial.flush();
        for(int i=0;i<3;i++)
        {
            string line;
            getline(serial,line);
            if(!line.empty() && line[line.length()-1]=='\r') line=line.substr(0,line.length()-1);
            if(line==ss.str()) return 0;
        }
        throw runtime_error("failed leaving firmware upgrade");
    }
    if(vm.count("keep"))
    {
        stringstream ss;
        ss<<vm["device"].as<int>()<<" "<<"k";
        serial<<ss.str()<<"\r";
        serial.flush();
        for(int i=0;i<3;i++)
        {
            string line;
            getline(serial,line);
            if(!line.empty() && line[line.length()-1]=='\r') line=line.substr(0,line.length()-1);
            if(line==ss.str()) return 0;
        }
        throw runtime_error("failed resetting the board");
    }
    return 0;
}
