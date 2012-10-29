clear; clc

//General parameters
T=60;                  //Sync period (1 minute)
N=5*24*60;             //Number of simulated sync periods (totaling 2 days)
nevents=500;           //Number of events that occur during the simulation
tick=1/16384;          //Resolution of one timer tick

//Clock skew, modeling manufacturing tolerances
//can simulate very well both node1 and node2 by just changing this parameter
//skew=16.01*tick;       //Skew (in seconds per sync period)
skew=16.51*tick;       //Skew (in seconds per sync period)
//Clock drift, modeling crystal temperature dependence in a day and night scenario
drifta=0.0008;         //Amplitude of drift sine (in seconds)
driftw=2*%pi/86400;    //Omega of drift sine
//Unpredictable clock jitter component
ltjittera=0.000035;    //Long term jitter amplitude (across synchronisations)
stjittera=0.000035;    //Short term jitter amplitude (within sync periods)

//Simulation-generated data
tsr=[0];               //Real time (root node time) at each synchronisation
tsm=[0];               //Receiver time when sync packet received
e=[0];                 //Error samples (equals inttime-tsr)
u=[0];                 //Actuator output
uq=[0];                //Actuator output (quantized)
inttime=[0];           //Real time when receiver expects the packet
events=[];             //Events expressed in real time (root node time)
mevents=[];            //Measured (receiver time) events
ieevents=[];           //Istantaneous estimate of events
epeevents=[];          //Ex post estimate of events

function res=quantize(x)
    res=int(x/tick+0.5)*tick;
endfunction

for k=2:N
    drift=drifta*(sin(driftw*k*T)-sin(driftw*(k-1)*T));
    jitter=rand(1,1,'normal')/3*ltjittera;

    tsr(k)=(k-1)*T;
    delta=skew+drift+jitter;
    tsm(k)=tsm(k-1)+T+delta;
    inttime(k)=inttime(k-1)+T-uq(k-1)+delta;
    e(k)=quantize(inttime(k)-tsr(k));
    if(e(k-1)==0)
        u(k)=uq(k-1)+1.25*e(k)-e(k-1);
    else
        u(k)=u(k-1)+1.25*e(k)-e(k-1);
    end
    uq(k)=quantize(u(k));
end

events=rand(1,nevents,'uniform')*T*(N-1);
for event=events
    kpre=find(tsr<=event);
    kpre=kpre($);
    kpost=kpre+1;

    //Quantities that the node knows to perform the sync
    tsrpre=tsr(kpre);
    tsrpost=tsr(kpost); //By definition tsrpre+T
    tsmpre=tsm(kpre);
    tsmpost=tsm(kpost);
    upre=uq(kpre);

    //Convert each event from real time to measured time, the result is what
    //the node will get by reading the timer when the event happens.
    //To do this use the full formula that considers the sinusoidal drift
    //from tsrpre to event and add another smaller jitter representing the
    //short term jitter within the synchronisation period of which only its
    //integral over a period is seen by the controller.
    mevent=tsmpre+((event-tsrpre)/(tsrpost-tsrpre))*(tsmpost-tsmpre);
    mevent=mevent+drifta*(sin(driftw*event)-sin(driftw*tsrpre));
    mevent=mevent+rand(1,1,'normal')/3*stjittera;
    mevents=[mevents,mevent];

    //Now go backwards using only the information known by the sensor node
    istantaneous=tsrpre+(mevent-tsmpre)/(T+upre)*T;
    ieevents=[ieevents,quantize(istantaneous)];
    expost=tsrpre+((mevent-tsmpre)/(tsmpost-tsmpre))*T;
    epeevents=[epeevents,quantize(expost)];
end

scf(0); clf;
subplot(211);
plot(tsm-tsr);
xlabel('Difference between receiver and transmitter time');
subplot(212);
plot(e);
xlabel('Synchronisation error');

scf(1); clf;
histplot(18,e/tick);
xlabel('Synchronisation error distribution');

qe=quantize(events);
eperror=(epeevents-qe)/tick;
ierror=(ieevents-qe)/tick;

scf(2); clf;
subplot(211);
plot(eperror);
xlabel('Ex post clock sync error');
subplot(212);
plot(ierror);
xlabel('Instantaneous clock sync error');

scf(3); clf;
subplot(211);
histplot([-4 -3 -2 -1 0 1 2 3 4],eperror);
xlabel('Error between transmitter and receiver timestamps (ex post)');
subplot(212);
histplot([-4 -3 -2 -1 0 1 2 3 4],ierror);
xlabel('Error between transmitter and receiver timestamps (istantaneous)');
//disp([epeevents-events;ieevents-events]);
