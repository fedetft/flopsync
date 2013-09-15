clear; clc; clf 
stacksize(100000000);

a1=fscanfMat('node1.dat');
a2=fscanfMat('node2.dat');
a3=fscanfMat('node3.dat');

function [tcomb,comb,terr,err]=composeData(a,settling,last)
    [q,p]=size(a);
    if last<0 then // last<0 means use all dataset
        last=q
    end
    d=a(settling:last,5:$);
    comb=[];
    for i=1:last-settling
        comb=[comb,d(i,:)];
    end
    tcomb=1:length(comb);
    tcomb=tcomb./(p-4);
    
    err=-a(:,1)';
    err=err(settling:last);
    terr=0:length(err)-1;
    
    printf("\nMean               (comb data) =%f\n",nanmean(comb));
    printf("Standard deviation (comb data) =%f\n",nanstdev(comb));
    printf("Mean               (e(t(k+1))-)=%f\n",nanmean(err));
    printf("Standard deviation (e(t(k+1))-)=%f\n",nanstdev(err));
endfunction

settling=10+1;
last=60;
[tcomb1,comb1,terr1,err1]=composeData(a1,settling,last);
[tcomb2,comb2,terr2,err2]=composeData(a2,settling,last);
[tcomb3,comb3,terr3,err3]=composeData(a3,settling,last);

subplot(411);
plot(tcomb1,comb1,'k');
plot(terr1,err1,'.k');
//plot(tcomb2,comb2,'b');
//plot(terr2,err2,'.b');
plot(tcomb3,comb3,'r');
plot(terr3,err3,'.r');
plot(terr1,zeros(terr1),':k');
ylabel("Sync error (us)","fontsize",2);
ax=get("current_axes");
ax.font_size=2;
ax.data_bounds=[0,-100;length(err1),500];

subplot(412);
plot(tcomb1,comb1,'k');
plot(terr1,err1,'.k');
plot(tcomb2,comb2,'b');
plot(terr2,err2,'.b');
//plot(tcomb3,comb3,'r');
//plot(terr3,err3,'.r');
plot(terr1,zeros(terr1),':k');
ylabel("Sync error (us)","fontsize",2);
ax=get("current_axes");
ax.font_size=2;
ax.data_bounds=[0,-20;length(err1),80];

b=fscanfMat('temperature.txt');
temp=b(settling:$,3);
temp1=a1(settling:last,4)'; temp1(1)=temp1(2); temp1=(-temp1+1872)/4+25.6;
temp2=a2(settling:last,4)'; temp2(1)=temp2(2); temp2=(-temp2+1921)/4.5+25.6;
temp3=a2(settling:last,4)'; temp3(1)=temp3(2); temp3=(-temp3+1922)/4.5+25.6;

subplot(413);
//plot(temp,'g');
plot(terr1,temp1,'k'); plot(terr2,temp2,'b'); plot(terr3,temp3,'r');
xlabel("Time (min)","fontsize",2);
ylabel("Temperature","fontsize",2);
ax=get("current_axes");
ax.font_size=2;
ax.data_bounds=[0,20;length(err1),50];
f=get("current_figure");
deltax=f.figure_size(1)-f.axes_size(1);
deltay=f.figure_size(2)-f.axes_size(2);
f.figure_size=[616*2+deltax,240/2*4+deltay];
