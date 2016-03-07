/**
 * Author: Terraneo Federico
 * Distributed under the Boost Software License, Version 1.0.
 */

#include "QAsyncSerial.h"
#include "AsyncSerial.h"
#include <QStringList>
#include <QRegExp>

/**
 * Implementation details of QAsyncSerial class.
 */
class QAsyncSerialImpl
{
public:
    QAsyncSerialImpl() : hasLast(false) {}
    CallbackAsyncSerial serial;
    unsigned char last;
    bool hasLast;
};

QAsyncSerial::QAsyncSerial(): pimpl(new QAsyncSerialImpl)
{
}

QAsyncSerial::QAsyncSerial(QString devname, unsigned int baudrate)
        : pimpl(new QAsyncSerialImpl)
{
    open(devname,baudrate);
}

void QAsyncSerial::open(QString devname, unsigned int baudrate)
{
    try {
        pimpl->serial.open(devname.toStdString(),baudrate);
    } catch(boost::system::system_error&)
    {
        //Errors during open
    }
    pimpl->serial.setCallback(bind(&QAsyncSerial::readCallback,this, _1, _2));
}

void QAsyncSerial::close()
{
    pimpl->serial.clearCallback();
    try {
        pimpl->serial.close();
    } catch(boost::system::system_error&)
    {
        //Errors during port close
    }
    pimpl->hasLast=false;
//     pimpl->receivedData.clear();//Clear eventual data remaining in read buffer
}

bool QAsyncSerial::isOpen()
{
    return pimpl->serial.isOpen();
}

bool QAsyncSerial::errorStatus()
{
    return pimpl->serial.errorStatus();
}

void QAsyncSerial::write(QString data)
{
    pimpl->serial.writeString(data.toStdString());
}

QAsyncSerial::~QAsyncSerial()
{
    pimpl->serial.clearCallback();
    try {
        pimpl->serial.close();
    } catch(...)
    {
        //Don't throw from a destructor
    }
}

double convert(unsigned char c1, unsigned char c2)
{
	static double bb=0.0;
	unsigned int dd=static_cast<unsigned int>(c1)<<8 | c2;
	dd=dd*208000/4095;
	
#define FILTER
	
#ifdef FILTER
	const double factor=0.95;
	bb=factor*bb+(1.0-factor)*dd;
#else //FILTER
	bb=dd;
#endif //FILTER
	
	return bb/1000;
}

void QAsyncSerial::readCallback(const char *data, size_t size)
{
	if(pimpl->hasLast)
	{
		pimpl->hasLast=false;
		emit(received(convert(pimpl->last,data[0])));
		data++;
		size--;
	}
	size_t rounded=(size & 1) ? (size-1) : size;
	for(size_t i=0;i<size/2;i+=2)
	{
		emit(received(convert(data[i],data[i+1])));
	}
    if(rounded!=size)
	{
		pimpl->hasLast=true;
		pimpl->last=data[size-1];
	}
}
