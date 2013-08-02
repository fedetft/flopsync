clear; clc; clf 

a=fscanfMat('node3.dat');
[q,w]=size(a);
d=a(:,5:$);

x=[];
for i=1:q
	x=[x,d(i,:)];
end
t=1:length(x);
t=t./(w-4);

e=a(:,1);
y=[0];
for i=2:q
	y=[y,e(i)];
end

tt=0:length(e)-1;
plot(t,-x,'b');
plot(tt,y,'.r');