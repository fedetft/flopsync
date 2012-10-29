clear; clc; clf

a=fscanfMat('node1.txt.dat');
b=fscanfMat('node2.txt.dat');

ea=a(:,2); wa=a(:,4);
eb=b(:,2); wb=b(:,4);

//ea=ea(1:7200); wa=wa(1:7200);
//eb=eb(1:7200); wb=wb(1:7200);

//ea=ea(1:1000); wa=wa(1:1000);
//eb=eb(1:1000); wb=wb(1:1000);

ta=0:length(ea)-1;
tb=0:length(eb)-1;

subplot(211);
ax=gca();
ax.font_size=4;
plot(ta,ea,'k',ta,wa,'r',ta,-wa,'r')

subplot(212);
ax=gca();
ax.font_size=4;
plot(tb,eb,'k',tb,wb,'r',tb,-wb,'r');

eaa=ea(10:$);
ebb=eb(10:$);
