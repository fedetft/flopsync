clear; clc

//General parameters

T=60;                      //Sync period (1 minute)
N=24*60;                   //Number of simulated sync periods (totaling 1 day)
nevents=500;               //Number of events that occur during the simulation

tick=1/16384;              //Resolution of one timer tick
packet_time=0.000056;      //Transmission time of one packet


//Clock skew, modeling manufacturing tolerances

skew1=16.51*tick;          //Skew of the first node (in seconds per sync period)
skew2=16.01*tick;          //Skew of the second node (in seconds per sync period)
skew12=(skew1-skew2)/2;    //Skew between nodes 1 and 2 (in seconds per half sync period)


//Clock drift, modeling crystal temperature dependence in a day and night scenario

drifta1=0.0008;            //Amplitude of drift sine (in seconds) - node 1
driftw1=2*%pi/86400;       //Omega of drift sine - node 1

drifta2=0.0008;            //Amplitude of drift sine (in seconds) - node 2
driftw2=2*%pi/86400;       //Omega of drift sine - node 2


//Unpredictable clock jitter component

ltjittera1=0.000035;       //Long term jitter amplitude (across synchronisations) - node 1
stjittera1=0.000035;       //Short term jitter amplitude (within sync periods)    - node 1

ltjittera2=0.000035;       //Long term jitter amplitude (across synchronisations) - node 2
stjittera2=0.000035;       //Short term jitter amplitude (within sync periods)    - node 2


//Simulation-generated data

tsr0=[0];                  //Real time (root node time) at each synchronisation
tsm1=[0];                  //Node 1 local time when sync packet is received
tsr1=[T/2];                //Node 1 corrected time when sync packet is sent
tsm2=[T/2];                //Node 2 local time when sync packet is received

e10=[0];                   //Radio sync error between nodes 0 and 1 (equals inttime1-tsr0)
e21=[0];                   //Radio sync error between nodes 1 and 2 (equals inttime2-tsr1)
e20=[0];                   //Radio sync error between node 1 and 2 (equals inttime2 - (tsr0+T/2))
u1=[0];                    //Actuator output - node 1
u2=[0];                    //Actuator output - node 2
uq1=[0];                   //Actuator output (quantized) - node 1
uq2=[0];                   //Actuator output (quantized) - node 2
inttime1=[0];              //Real time when node 1 expects the packet
inttime2=[T/2];            //Real time when node 2 expects the packet

events=[];                 //Events expressed in real time (root node time)
mevents1=[];               //Measured (node 1 local time) events
mevents2=[];               //Measured (node 2 local time) events
ieevents1=[];              //Istantaneous estimate of events - node 1
ieevents2=[];              //Istantaneous estimate of events - node 2
epeevents1=[];             //Ex post estimate of events - node 1
epeevents2=[];             //Ex post estimate of events - node 2


//Receiving window dimension parameter

wsampsize=60;              //Size of sample window
multiplier=3;              //Factor multiplying the variance in the computation of the receiving window size
w_min=3*tick;              //Minimum value of the dimension of the receiving window
w_max=0.003;               //Maximum value of the dimension of the receiving window

w1sizes=[w_max];           //Dimension of the receiving window (node 1)
w2sizes=[w_max];           //Dimension of the receiving window (node 2) 


//Parameters for the function wtime
str1 = struct('esum', 0, 'esqsum', 0, 'wsize', w_max, 'i', 0);
str2 = struct('esum', 0, 'esqsum', 0, 'wsize', w_max, 'i', 0);


//Power consumption model

a1=0.0000149;
b1=0.00001358;
c1=0.01479;
d1=0;

a2=0.0000149;
b2=0.00001345;
c2=0.01659;
d2=0.01659;

a3=0.0000149;
b3=0.00002703;
c3=0.03137;
d3=0.01659;

iavg0= a1 + b1/T + c1*packet_time/T;                    //Average current consumption of node 0
iavg1=[a2 + b2/T + c2*packet_time/T + d2*w_max/T];      //Average current consumption single hop
iavg2=[a3 + b3/T + c3*packet_time/T + d3*w_max/T];      //Average current consumption multi hop (node 1 and 2)


//Easy choice of the desired contributions

skew1 = skew1 * 1;
skew2 = skew2 * 1;

skew12=(skew1-skew2)/2;

drifta1 = drifta1 * 1;
drifta2 = drifta2 * 1;

ltjittera1 = ltjittera1 * 1;
stjittera1 = stjittera1 * 1;

ltjittera2 = ltjittera2 * 1;
stjittera2 = stjittera2 * 1;

JITTERS = 1;


//Support functions

function res=quantize(x)
    res=int(x/tick+0.5)*tick;
endfunction

function res=wtime(str, ewind)                                                            //It computes the dimension of the receiving window
    ewind=ewind/tick;
    str.esum = str.esum + ewind;
    str.esqsum = str.esqsum + ewind^2;
    str.i = str.i + 1;
    
    if (str.i == wsampsize) then
        str.i = 0;
        var = str.esqsum/wsampsize - (str.esum/wsampsize)^2;
        str.wsize = int((min(w_max/tick, max(w_min/tick, multiplier*var))) + 0.5);        //Here we used int instead of quantize because quantize works with seconds
        str.wsize = str.wsize * tick;
        str.esum = 0;
        str.esqsum = 0;
    end
    
    res = str;
    
endfunction

function verPeaks(kpre, kpost, event, expost2)                                            //Check on ex-post: (print some info if the quantized error is > 2)
    qerr = quantize(expost2);
    qe = quantize(event);
    y = (qerr-qe)/tick
    
    if (y>2 | y<-2) then
        disp(y);
        disp(e21(kpre)/tick, e21(kpost)/tick);
        disp(e10(kpre)/tick, e10(kpost)/tick);
        disp("\n");
    end
endfunction

function verBugWind(k)                                                                                       //Check on errors e21 and e10: print the step k where
    if (quantize(abs(e10(k)))>quantize(str1.wsize) | quantize(abs(e21(k)))>quantize(str2.wsize)) then           //the errors exceed the receiving window dimension
        disp (k);
    end
endfunction


//Radio synchronization

for k=2:N
    drift1=drifta1*(sin(driftw1*k*T)-sin(driftw1*(k-1)*T));
    jitter1=rand(1,1,'normal')/3*ltjittera1 * JITTERS;

    //Node 0
    tsr0(k)=(k-1)*T;                                                    //Real time
    
    //Node 1
    delta1=skew1+drift1+jitter1;                                        //Total error produced by all the contributions
    tsm1(k)=tsm1(k-1)+T+delta1;                                         //Local time
    inttime1(k)=inttime1(k-1)+T-uq1(k-1)+delta1;                        //Corrected time
    e10(k)=quantize(inttime1(k)-tsr0(k));                               //Radio sync error between nodes 0 and 1
    if(e10(k-1)==0)                                                     //u1 = correction computed by the controller
        u1(k)=uq1(k-1)+1.25*e10(k)-e10(k-1);
    else
        u1(k)=u1(k-1)+1.25*e10(k)-e10(k-1);
    end
    uq1(k)=quantize(u1(k));
    
    
    drift12=drifta1*(sin(driftw1*(k-0.5)*T)-sin(driftw1*(k-1)*T));
    jitter12=rand(1,1,'normal')/3*ltjittera1 * JITTERS;
    
    delta12=skew12+drift12+jitter12                                     //Total error produced in half period between node 1 and 2 (node 1 sends sync packet at T/2)
    
    correz = inttime1(k)-inttime1(k-1);                                 //Correction applied by node 1 (in sending sync packets)
    
    tsr1(k) = tsr0(k) + T/2*(T/correz) + delta12;                       //Global time of: node 1 sends sync packet
   
    //Node 2
    drift2=drifta2*(sin(driftw2*k*T)-sin(driftw2*(k-1)*T));
    jitter2=rand(1,1,'normal')/3*ltjittera2 * JITTERS;
    delta2=skew2+drift2+jitter2;                                        //Total error produced by all the contributions    
    tsm2(k)=tsm2(k-1)+T+delta2;                                         //Local time
    inttime2(k)=inttime2(k-1)+T-uq2(k-1)+delta2;                        //Corrected time
    
    e21(k)=quantize(inttime2(k)-tsr1(k));                               //Radio sync error between nodes 1 and 2  
    e20(k)=quantize(inttime2(k)-(tsr0(k)+T/2));                         //Radio sync error between nodes 0 and 2
    
    if(e21(k-1)==0)                                                     //u2 = correction computed by the controller
        u2(k)=uq2(k-1)+1.25*e21(k)-e21(k-1);
    else
        u2(k)=u2(k-1)+1.25*e21(k)-e21(k-1);
    end
    uq2(k)=quantize(u2(k));
    
    
    //Update of the receiving windows dimensions
    str1 = wtime(str1, e10(k));
    str2 = wtime(str2, e21(k));
    
    //If the error is bigger than the window size, we double the window size (always within w_max)
    if (quantize(abs(e10(k)))>quantize(str1.wsize)) then
        str1.wsize=min (str1.wsize*2, w_max);
    end
    
    if (quantize(abs(e21(k)))>quantize(str2.wsize)) then
        str2.wsize=min (str2.wsize*2, w_max);
    end
    
    w1sizes(k)= str1.wsize;
    w2sizes(k)= str2.wsize;

    
    //Check that errors are not bigger than windows
    verBugWind(k);
    
    
    //Update of average current consumptions
    iavg1(k)= a2 + b2/T + c2*packet_time/T + d2*str1.wsize/T;
    iavg2(k)= a3 + b3/T + c3*packet_time/T + d3*str2.wsize/T;
    
        
end


//Clock synchronization

events=rand(1,nevents,'uniform')*T*(N-1);
events=gsort(events, 'g', 'i');
for event=events
    
    kpre=find(tsr0<=event);
    kpre=kpre($);
    kpost=kpre+1;

    //Quantities that the node knows to perform the sync
    tsrpre=tsr0(kpre);
    tsrpost=tsr0(kpost); //By definition tsrpre+T
    
    tsm1pre=tsm1(kpre);
    tsm1post=tsm1(kpost);
    upre1=uq1(kpre);
    
    tsm2pre=tsm2(kpre);
    tsm2post=tsm2(kpost);
    upre2=uq2(kpre);

    //Convert each event from real time to measured time, the result is what
    //the nodes (1 and 2) will respectively get by reading the timer when the event happens.
    //To do this use the full formula that considers the sinusoidal drift
    //from tsrpre to event and add another smaller jitter representing the
    //short term jitter within the synchronisation period of which only its
    //integral over a period is seen by the controller.
    mevent1=tsm1pre+((event-tsrpre)/(tsrpost-tsrpre))*(tsm1post-tsm1pre);
    mevent1=mevent1+drifta1*(sin(driftw1*event)-sin(driftw1*tsrpre));
    mevent1=mevent1+rand(1,1,'normal')/3*stjittera1;
    mevents1=[mevents1,mevent1];
    
    mevent2=tsm2pre+((event-tsrpre)/(tsrpost-tsrpre))*(tsm2post-tsm2pre);
    mevent2=mevent2+drifta2*(sin(driftw2*event)-sin(driftw2*tsrpre));
    mevent2=mevent2+rand(1,1,'normal')/3*stjittera2;
    mevents2=[mevents2,mevent2];

    //Now go backwards using only the information known by the sensor node
    istantaneous1=tsrpre+(mevent1-tsm1pre)/(T+upre1)*T;
    ieevents1=[ieevents1,quantize(istantaneous1)];
    expost1=tsrpre+((mevent1-tsm1pre)/(tsm1post-tsm1pre))*T;
    epeevents1=[epeevents1,quantize(expost1)];
    
    istantaneous2=tsrpre+(mevent2-tsm2pre)/(T+upre2)*T;
    ieevents2=[ieevents2,quantize(istantaneous2)];
    expost2=tsrpre+((mevent2-tsm2pre)/(tsm2post-tsm2pre))*T;
    epeevents2=[epeevents2,quantize(expost2)];
    
    
    //Check peaks
    verPeaks(kpre, kpost, event, expost2);
    
    
end


//Plots

x = 1:length(e10);
x = x';

qe=quantize(events);
eperror1=(epeevents1-qe)/tick;
ierror1=(ieevents1-qe)/tick;
eperror2=(epeevents2-qe)/tick;
ierror2=(ieevents2-qe)/tick;

//Radio Synch Nodo1-Nodo0
scf(0); clf;
subplot(211);
plot((tsm1-tsr0));
title('Radio Synch Nodo1-Nodo0');
xlabel('Difference between receiver and transmitter time: tsm1-tsr0');
subplot(212);
plot(x, [e10,w1sizes,-w1sizes]);
xlabel('Synchronisation error (e10) and receiving windows (w1sizes, -w1sizes)');

//Radio Synch Nodo1-Nodo0 error distribution
scf(1); clf;
histplot(18,e10/tick);
xlabel('Synchronisation error distribution');
title('Radio Synch Nodo1-Nodo0 error distribution');

//Clock Synch Nodo1-Nodo0
scf(2); clf;
subplot(211);
plot(eperror1);
xlabel('Ex post clock sync error');
title('Clock Synch Nodo1-Nodo0');
subplot(212);
plot(ierror1);
xlabel('Instantaneous clock sync error');

//Clock Synch Nodo1-Nodo0 error distribution
scf(3); clf;
subplot(211);
histplot([-4 -3 -2 -1 0 1 2 3 4],eperror1);
xlabel('Error between transmitter and receiver timestamps (ex post)');
title('Clock Synch Nodo1-Nodo0 error distribution');
subplot(212);
histplot([-4 -3 -2 -1 0 1 2 3 4],ierror1);
xlabel('Error between transmitter and receiver timestamps (istantaneous)');


//Radio Synch Nodo2-Nodo1
scf(4); clf;
subplot(211);
plot((tsm2-(tsm1+T/2)));
xlabel('tsm2-tsm1');
title('Radio Synch Nodo2-Nodo1');
subplot(212);
plot(x, [e21, w2sizes, -w2sizes]);
xlabel('e21: inttime2-tsr1 e receiving windows (w2sizes, -w2sizes)');

//Radio Synch Nodo2-Nodo0
scf(5); clf;
subplot(211);
plot((tsm2-(tsr0+T/2)));
xlabel('tsm2-(tsr0+T/2)');
title('Radio Synch Nodo2-Nodo0');
subplot(212);
plot(e20);
xlabel('e20: inttime2-(tsr0+T/2)');

//Radio Synch rx0-tx1
scf(6); clf;
plot((tsr1-(tsr0+T/2)));
xlabel('tsr1-(tsr0+T/2)');
title('Radio Synch rx0-tx1');

//Clock Synch Nodo2-Nodo0
scf(7); clf;
subplot(211);
plot(eperror2);
xlabel('Ex post clock sync error - NODO 2');
title('Clock Synch Nodo2-Nodo0');
subplot(212);
plot(ierror2);
xlabel('Instantaneous clock sync error - NODO 2');

//Clock Synch Nodo2-Nodo0 error distribution
scf(8); clf;
subplot(211);
histplot([-4 -3 -2 -1 0 1 2 3 4],eperror2);
xlabel('Error between transmitter and receiver timestamps (ex post)');
title('Clock Synch Nodo2-Nodo0 error distribution');
subplot(212);
histplot([-4 -3 -2 -1 0 1 2 3 4],ierror2);
xlabel('Error between transmitter and receiver timestamps (istantaneous)');

//Power consumption nodes 1 and 2
scf(9); clf;
subplot(211);
plot(iavg1);
xlabel('iavg single hop');
subplot(212);
plot(iavg2);
xlabel('iavg multi hop');

//disp(iavg0);
//disp(mean(iavg1));
//disp(mean(iavg2));

//Easy way to check abs(e21)<w1sizes and abs(e10)<w1sizes through points
xa=[0];
e21a=[0];
w2sizesa=[0];
start=1100;

for k=start:start+100
    xa(k-(start-1)) = x(k);
    e21a(k-(start-1)) = e21(k);
    w2sizesa(k-(start-1)) = w2sizes(k);
end;

xa=xa';

scf(10); clf;
plot (xa, [e21a, w2sizesa, -w2sizesa], 'o');