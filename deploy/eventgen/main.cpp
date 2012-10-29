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
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include "serialstream.h"

using namespace std;
using namespace boost;
using namespace boost::program_options;

int main(int argc, char* argv[])
{
    options_description desc;
    desc.add_options()
        ("help","Prints this")
        ("port",value<string>(),"Serial port to connect to")
        ("baudrate",value<int>(),"Serial port baudrate")
        ("mininterval",value<int>(),"Minimum event inteval")
        ("maxinterval",value<int>(),"Maximum event inteval")
        ("file",value<string>(),"File where to save the data")
    ;
    variables_map vm;
    store(parse_command_line(argc,argv,desc),vm);
    notify(vm);
    if(vm.count("help")) { cout<<desc<<endl; return 0; }
    if(!vm.count("port")) throw runtime_error("No serial port is given");
    if(!vm.count("baudrate")) throw runtime_error("No baudrate is given");
    if(!vm.count("file")) throw runtime_error("No file is given");
    if(!vm.count("mininterval")) throw runtime_error("No file mininterval");
    if(!vm.count("maxinterval")) throw runtime_error("No file maxinterval");

    const int mininterval=vm["mininterval"].as<int>();
    const int maxinterval=vm["maxinterval"].as<int>();
    if(mininterval<=0 || maxinterval<=0 || maxinterval<mininterval)
        throw runtime_error("Interval error");
    mt19937 rng(time(NULL));
    uniform_int<> range(mininterval,maxinterval);
    variate_generator<mt19937&,uniform_int<> > random(rng,range);

    SerialOptions options;
    options.setDevice(vm["port"].as<string>());
    options.setBaudrate(vm["baudrate"].as<int>());

    ofstream out(vm["file"].as<string>().c_str());

    for(;;)
    {
        int r=random();
        out<<r<<endl;
        out.flush();
        this_thread::sleep(posix_time::seconds(r));
        SerialStream serial(options);
        serial.exceptions(ios::badbit | ios::failbit); //Important!
        serial<<"e\r";
        serial.flush();
        string line;
        getline(serial,line);
        serial.close();
        if(!line.empty() && line[line.length()-1]=='\r') line=line.substr(0,line.length()-1);
        if(line!="event")
            throw runtime_error(string("Arduino error: got ")+line);
    }
    return 0;
}
