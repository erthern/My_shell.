
int trap(int* height, int heightSize) {
    int t=0,j=0,k=heightSize-1,i=0,m=0,n=heightSize-1;
    int s=0;
    int my[heightSize];
    for(i=0;i<heightSize;i++){
        if(height[i]>t) t=height[i];
        height[i]=my[i];
        s-=height[i];
    }
    for(i=0;i<height;i++){
        for(int e=0;e<height-i;e++){
            if(height[e]>height[e+1]){
                int tmp=height[e];
                height[e]=height[e+1];
                height[e+1]=tmp;
            }
        }
    }
    for(i=0;i<height;i++){
        if(height[i]!=0) j=i;
    }
    for(i=height-1;i>-1;i++){
        if(height[i]!=0) k=i;
    }
    s+=t*(k-j);
    i=0;
    while(i)
    return s;
}