#include <iostream>
#include <vector>
#include "predict.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#define random(a,b) (rand()%(b-a+1)+a)
//你要完成的功能总入口
using namespace std;
struct 
{
	int cpu;
	int mem, driver;
}physical;
int M_cpu, M_mem;

#define max_machine 5000

float alpha = 0.0041;//算法关键系数
float noise_reduction_index = 3;
float abandon_index=0.3;

int virtual_type[15][2] =									// vitual machine type infomation {id , cpu ,mem(GB)}
				{{ 1, 1 }, { 1, 2 }, {1, 4 }, {2, 2 },
				{ 2, 4 }, {2, 8 }, {4, 4 }, {4, 8 }, 
				{ 4, 16 },{8, 8 }, {8, 16 }, { 8, 32 }, 
				{ 16, 16 }, {16, 32 }, {16, 64 }};

typedef struct time
{
	time(int y,char m,char d,char h,char mi,char s):year(y),month(m),day(d),hour(h),minute(mi),second(s){}
	time() :year(0), month(0), day(0), hour(0), minute(0), second(0){}
	int year;
	char month, day, hour, minute, second;
	void print_date()
	{	printf("%d %d %d %d %d %d", year, month, day, hour, minute, second);	}
}date;
 
typedef struct
{
	int pair[2];
}
vitual_pair;
//vitual_pair vit_int = { 0, 0 };
typedef struct :time
{
//	char mon, day;
//	oneday() :vit_pair(1, {0,0}){}
	vector<vitual_pair> vit_pair;
}date_;

vector<date_> tran_data;

date period[2];
date tran_period[2];
int day_predict,day_tran;



char vitual = 0,*vit_type;	//vitual : how many vitual type needed vit_type :the id of each type (map to vitual)
//int *map_num;					//the final num of each vitual type(map to vitual)
int vit_num;						//result of num of all virtual machine
int *num_predict;				//result of num of each vitual needed
int N_machine = 0;//物理机数量

int **flavor_and_day;//行数为虚拟机种类，列数为每日的量（叠加）

enum {CPU = 0, MEM = 1} bias;

int phy_num , *phy_type ,**phy_need;

typedef int virtual_dep[3];
virtual_dep* virtual_deploy;

int N_cpu = 0, N_mem = 0, i, j ,k= 0;//已使用的cpu和mem量
struct machine{
	int info[15];
}Machine_array[max_machine];//利用结构体存储信息

int isempty(int i){//判断i种类型虚拟机是否为0 
	if (virtual_deploy[i][0] <= 0){
		//virtual_deploy[i][1]=0;
		//virtual_deploy[i][2]=0;//需求量为0则全置0
		return 0;
	}
	else
		return 1;
}

int isfull(){//判断单台物理机上超了没 
	if (N_cpu>M_cpu || N_mem>M_mem)
		return 0;
	else
		return 1;
}
int addisfull(int i){//加上此台虚拟机会超过物理机容量吗
	int N_cpu_temp = 0, N_mem_temp = 0;
	N_cpu_temp = N_cpu + virtual_deploy[i][1];
	N_mem_temp = N_mem + virtual_deploy[i][2];
	if (N_cpu_temp>M_cpu || N_mem_temp>M_mem)
		return 0;
	else
		return 1;
}
int canbreak(){//所有的虚拟机都布置完了 
	int flavor_temp;
	flavor_temp = vitual;
	for (i = 0; i<vitual; i++)
	if (virtual_deploy[i][0] <= 0)
		flavor_temp--;
	if (flavor_temp <= 0)
		return 0;
	else
		return 1;
}

int canremove(int machine){//能否删去这台物理机 
	float cpu_use = 0, mem_use = 0;
	for (i = 0; i<vitual; i++){
		float cpu_use_temp, mem_use_temp;
		cpu_use_temp = (float)Machine_array[machine].info[i] * virtual_deploy[i][1];
		mem_use_temp = (float)Machine_array[machine].info[i] * virtual_deploy[i][2];

		cpu_use = cpu_use + cpu_use_temp;
		mem_use = mem_use + mem_use_temp;
	}
	if (cpu_use<abandon_index*M_cpu && mem_use<abandon_index*M_mem){
		for (i = 0; i<vitual; i++)
			virtual_deploy[i][0] = virtual_deploy[i][0] + Machine_array[machine].info[i];
		N_machine--;
		return 0;
	}
	else
		return 1;
}

void deploy_method(void){
	N_machine = 0;
    //cout<<"�ѽ���deploy_method����"<<endl;
	float rate1, rate2;
	float compare[20] = { 0 };

	while (canbreak()){
	    //cout<<"�ѽ���deploy_method�������ѭ��"<<endl;
		int max_cpu = 0, max_mem = 0;
	
		for (i = 0; i<vitual; i++){

			if (virtual_deploy[i][1+bias]>max_cpu && isempty(i)){ 
				max_cpu = virtual_deploy[i][1+bias];
				max_mem = virtual_deploy[i][2-bias];
				j = i;
			}		
		}
		virtual_deploy[j][0]--;//该种虚拟机数量减小
		Machine_array[N_machine].info[j]++;
		if(bias==0)
			N_cpu = N_cpu + max_cpu, N_mem = N_mem + max_mem;
		else
			N_cpu = N_cpu + max_mem, N_mem = N_mem + max_cpu;

		while (canbreak()){
			
			int flag = 0;
			for (i = 0; i<vitual; i++){
				if (isempty(i) && addisfull(i)){

					rate1 = (float)(virtual_deploy[i][1] + N_cpu) / M_cpu;
					rate2 = (float)(virtual_deploy[i][2] + N_mem) / M_mem;
					compare[i] = fabs(rate2 - rate1);
					rate1 = 0, rate2 = 0;
					flag++;
				}

			}
			if (flag == 0)
				break;
		
			j = 0;
			float compare_temp = 2.0;
			for (i = 0; i<vitual; i++){//求最小的误差
				if (compare[i]<compare_temp && compare[i] != 0){
					compare_temp = compare[i];
					j = i;
				}

			}
			//将compare归0
			for (i = 0; i<20; i++)
				compare[i] = 0.0;

			N_cpu = N_cpu + virtual_deploy[j][1];
			N_mem = N_mem + virtual_deploy[j][2];//先加上去
			if (isfull() == 0)
				break;//退出循环                            //这句未被执行 
			virtual_deploy[j][0]--;//该种虚拟机数量
			// printf("virtual_deploy[%d][0]=%d\n",j,virtual_deploy[j][0]);
			Machine_array[N_machine].info[j]++;
		}

		if (canremove(N_machine) == 0)
			break;
		N_machine++;//物理机数量增加
		N_cpu = 0, N_mem = 0;
		printf("the number of physcal machine is %d,TIAOCHUTIAOJIAN is %d\n", N_machine,canbreak());
	}
	for (i = 0; i<vitual; i++) {
		cout<<"vitual number is "<<virtual_deploy[i][0]<<endl;
	}
	printf("Now the number of physcal machine is %d", N_machine);
   //cout<<"exit function deploy_method outside circullar"<<endl;
   
   //add virtual machine to improve utilazation //2018/3/20
   int flag_com = 1;
    N_machine=N_machine-1;
	float cpu_use = 0, mem_use = 0;
	float cpu_use_temp=0, mem_use_temp=0;
	for (i = 0; i<vitual; i++) {
		cpu_use_temp=0, mem_use_temp=0;
		cpu_use_temp = (float)Machine_array[N_machine].info[i] * virtual_deploy[i][1];
		mem_use_temp = (float)Machine_array[N_machine].info[i] * virtual_deploy[i][2];

		cpu_use = cpu_use + cpu_use_temp;
		mem_use = mem_use + mem_use_temp;
	}
	N_cpu = cpu_use;
	N_mem = mem_use;

	int flag1 = 0;
	while (flag_com){

			flag1 = 0;
			for (i = 0; i<vitual; i++){
				if (addisfull(i)){
					rate1 = (float)(virtual_deploy[i][1] + N_cpu) / M_cpu;
					rate2 = (float)(virtual_deploy[i][2] + N_mem) / M_mem;
					compare[i] = fabs(rate2 - rate1);
					rate1 = 0, rate2 = 0;
					flag1++;
				}

			}
			if (flag1 == 0)
				break;
		
			j = 0;
			float compare_temp = 2.0;
			for (i = 0; i<vitual; i++){//求最小的误差
				if (compare[i]<compare_temp && compare[i] != 0){
					compare_temp = compare[i];
					j = i;
				}

			}
			for (i = 0; i<20; i++)
				compare[i] = 0.0;

			N_cpu = N_cpu + virtual_deploy[j][1];
			N_mem = N_mem + virtual_deploy[j][2];//先加上去
			Machine_array[N_machine].info[j]++;
			
		int N_cpu_temp = 0, N_mem_temp = 0;
		for (i = 0; i<vitual; i++){
			N_cpu_temp = N_cpu + virtual_deploy[i][1];
			N_mem_temp = N_mem + virtual_deploy[i][2];
			if (N_cpu_temp<=M_cpu && N_mem_temp<=M_mem)
				{flag_com = 1;}
				
			N_cpu_temp = 0, N_mem_temp = 0;
		}
	}
			
	N_cpu = 0;
	N_mem = 0;
	N_machine=N_machine+1;
	
	//calculate the total ultilization of CPU  //2018/4/8
	float cpu_temp = 0,utilization_cpu = 0,utilization_mem = 0;
	for (i = 0; i<N_machine; i++) 
	{
		for (j = 0; j < vitual; j++) 
		{
				cpu_temp = cpu_temp + (float)Machine_array[i].info[j] * virtual_deploy[j][1];
		}
	}
	utilization_cpu= (float)cpu_temp/(M_cpu*N_machine);
 
	//calculate the total ultilization of MEMORY  //2018/4/8
	float mem_temp = 0;
	for (i = 0; i<N_machine; i++)
	{
		for (j = 0; j < vitual; j++)
		{
			mem_temp = mem_temp + (float)Machine_array[i].info[j] * virtual_deploy[j][2];
		}
	}
	utilization_mem = (float)mem_temp /(M_mem*N_machine);

    cout<<"the utilization of mem is "<<utilization_mem<<endl;
    cout<<"the utilization of cpu is "<<utilization_cpu<<endl;
}

void finally_deploy(char *file)
{
	int temp = 0;
	int virtual_arry[15] = { 0 };
	for (i = 0; i<N_machine; i++){
		for (j = 0; j < 15; j++){
			//			printf("info  is %d %d\n", virtual_type[j][0],virtual_type[j][1]);
			if (Machine_array[i].info[j] != 0)
			{
				//				printf("machine %d has %d type %d\n", i + 1, Machine_array[i].info[j], j +1);
				temp = temp + Machine_array[i].info[j];
			}
		}
	}

	for (i = 0; i< 15 ; i++){
		for (j = 0; j<N_machine; j++)
			virtual_arry[i] = virtual_arry[i] + Machine_array[j].info[i];
	}
	//	printf("the total num of virtual machine is %d\n", temp);
	/*	for (i = 0; i<15; i++){
	if (virtual_arry[i] != 0)
	printf("%d type has %d ge\n", i+1, virtual_arry[i]);
	}
	*/

	FILE *fp = fopen(file, "a");
	if (fp == NULL)
	{
		printf("Fail to open file %s, %s.\n", file, strerror(errno));
		return;
	}

	//	cout << "there ok" << endl;
	fprintf(fp, "%d", temp); fputc('\n', fp);
	for (int count = 0; count < vitual; count++)
	{
		fputs("flavor", fp); fprintf(fp, "%d", vit_type[count]); fputc(' ', fp);
		fprintf(fp, "%d", virtual_arry[count]); fputc('\n', fp);
	}
	fputc('\n', fp);
	fprintf(fp, "%d", N_machine); fputc('\n', fp);
	/*
	for (j = 0; j<15; j++)
	{	fprintf(fp, "%d", j + 1);
	for (i = 0; i<N_machine; i++){

	if (Machine_array[i].info[j] != 0)
	{
	fputc(' ', fp); fputs("flavor", fp); fprintf(fp, "%d", j);
	fputc(' ', fp); fprintf(fp, "%d", Machine_array[i].info[j]);
	}
	fputc('\n', fp);
	}
	}*/
	for (i = 0; i < N_machine; i++)
	{
		fprintf(fp, "%d", i + 1);
		for (j = 0; j < vitual; j++)
		{
			if (Machine_array[i].info[j] != 0)
			{
				int count3 = 0; bool flag = 0;
				//				while (j != vit_type[count3++]) if (count3 >= vitual) { flag = 1; break; }
				if (!flag)
				{
					fputc(' ', fp); fputs("flavor", fp); fprintf(fp, "%d", vit_type[j]);
					fputc(' ', fp); fprintf(fp, "%d", Machine_array[i].info[j]);
				}
			}
		}
		fputc('\n', fp);
	}
	//for (int count = 0; count < N_machine; count++)
	//{
	//	
	//	for (int count2 = 0; count2 < phy_type[count]; count2++)
	//	{
	//
	//	}
	//	
	//}
}
void flavor_predict(void)
{
	int i, j;
	float* rms_flavor_and_day = (float*)malloc(vitual*sizeof(float));
	float* mean_flavor_and_day = (float*)malloc(vitual*sizeof(float));//average
	for(i = 0; i<vitual; i++)
	{rms_flavor_and_day[i] = 0;
	 mean_flavor_and_day[i] = 0;}
	//noise reduction             //2018.3.27
	//get the independent number of vm everyday
	for (i = 0; i<vitual; i++){
	    for (j = tran_data.size()-1; j>0; j--){
	     flavor_and_day[i][j]=flavor_and_day[i][j]-flavor_and_day[i][j-1];
        } 
    }
    //calculate RMS
    for (i = 0; i<vitual; i++){
	    for (j = 0; j<tran_data.size(); j++){
	     rms_flavor_and_day[i]=rms_flavor_and_day[i]+flavor_and_day[i][j]*flavor_and_day[i][j];
	     mean_flavor_and_day[i]=mean_flavor_and_day[i]+flavor_and_day[i][j];
        } 
        rms_flavor_and_day[i]=sqrt(rms_flavor_and_day[i]/tran_data.size());
        mean_flavor_and_day[i]=mean_flavor_and_day[i]/tran_data.size();
        //printf(" rms is %f \n", rms_flavor_and_day[i]);
    }
	//concrete code of noise reduction
	for (i = 0; i<vitual; i++){
	    for (j = 0; j<tran_data.size(); j++){
	    	if(flavor_and_day[i][j]>= (noise_reduction_index * rms_flavor_and_day[i]))
	    	{
	    		if(j==0 || j==(tran_data.size()-1))
	    		    flavor_and_day[i][j]=0;//(int)(mean_flavor_and_day[i]+1);//int(rms_flavor_and_day[i]+1);
	    		else
	    		    flavor_and_day[i][j]=0;//(flavor_and_day[i][j-1]+flavor_and_day[i][j+1])/2;
			}
	            
        } 
    }
    //Mean Filter
    //transmit int matrix to float matrix
    float **float_flavor_and_day;
    float_flavor_and_day=(float**)malloc(sizeof(float*)*vitual);    
    for(i=0;i<vitual;i++)
    {
	float_flavor_and_day[i]=(float*)malloc(sizeof(float)*tran_data.size());
	} 
    for (i = 0; i<vitual; i++){
	    for (j = 0; j<tran_data.size(); j++){
	    	if(j>1 && j<(tran_data.size()-2))
	    	{
			float_flavor_and_day[i][j]=(float)(flavor_and_day[i][j-2]+flavor_and_day[i][j-1]+flavor_and_day[i][j+1]+flavor_and_day[i][j+2]);
				//float_flavor_and_day[i][j]=(float)(flavor_and_day[i][j-1]+flavor_and_day[i][j+1]);
				float_flavor_and_day[i][j]=float_flavor_and_day[i][j]/4;
			}
			else
			{
				float_flavor_and_day[i][j]=(float)flavor_and_day[i][j];
			}
			//printf("float_flavor_and_day is %f \n", float_flavor_and_day[i][j]);
        } 
    }
    //restore the matrix "float_flavor_and_day" after noise reduction
	for (i = 0; i<vitual; i++){
	    for (j = 1; j<tran_data.size(); j++){
	     float_flavor_and_day[i][j]=float_flavor_and_day[i][j]+float_flavor_and_day[i][j-1];
        } 
    }
    /*
	//restore the matrix "flavor_and_day" after noise reduction
	for (i = 0; i<vitual; i++){
	    for (j = 1; j<tran_data.size(); j++){
	     flavor_and_day[i][j]=flavor_and_day[i][j]+flavor_and_day[i][j-1];
        } 
    }
    */
    free(rms_flavor_and_day);
    free(mean_flavor_and_day);
   
    
	//predict                  
	for (i = 0; i<vitual; i++){
		
		float* st1 = (float*)malloc(tran_data.size()*sizeof(float));
		float* st2 = (float*)malloc(tran_data.size()*sizeof(float));
		//float st1[tran_data.size()], st2[tran_data.size()];//????1‰?–1???é”?èˉˉ
		st1[0] = float_flavor_and_day[i][0];
		st2[0] = float_flavor_and_day[i][0];

		for (j = 1; j<tran_data.size(); j++){
			st1[j] = alpha* float_flavor_and_day[i][j] + (1 - alpha)* st1[j - 1];
			st2[j] = alpha* st1[j] + (1 - alpha)* st2[j - 1];
		}
		float k, b;
		b = 2 * st1[tran_data.size() - 1] - st2[tran_data.size() - 1];
		k = alpha / (1 - alpha)*(st1[tran_data.size() - 1] - st2[tran_data.size() - 1]);
		free(st1), free(st2);
		num_predict[i] = (int)(k*day_predict + b + 0.5f)+ random(1,5);//四舍五入	  
		//printf("k/b is %f %f\n",k,b);
	}
	for (i = 0; i < vitual; i++) printf("num_predict is %d \n",num_predict[i]);
	free(float_flavor_and_day);
}

void var_int(void)
{
	virtual_deploy= new virtual_dep[vitual];
	for (int count = 0; count < vitual; count++)
	{
		virtual_deploy[count][0] = num_predict[count];
		printf("%d ,%d", virtual_deploy[count][1] = virtual_type[vit_type[count] - 1][0], vit_type[count]); printf("true is %d %d" ,virtual_type[2][0],virtual_type[2][1]);
		printf("%d \n" ,virtual_deploy[count][2] = virtual_type[vit_type[count] - 1][1]);
	}
	M_cpu = physical.cpu;
	M_mem = physical.mem;
}
void get_date(char *date_point,date *where)
{
	/*int count = 0;
	while (*date_point <= '9' && *date_point >= '0')
		count = count * 10 + *date_point++ - '0';
	where->year = count;
	count = 0; date_point++;
	while (*date_point <= '9' && *date_point >= '0')
		count = count * 10 + *date_point++ - '0';
	where->month = count;
	count = 0; date_point++;
	while (*date_point <= '9' && *date_point >= '0')
		count = count * 10 + *date_point++ - '0';
	where->day = count;
	count = 0; date_point++;
	while (*date_point <= '9' && *date_point >= '0')
		count = count * 10 + *date_point++ - '0';
	where->hour = count;
	count = 0; date_point++;
	while (*date_point <= '9' && *date_point >= '0')
		count = count * 10 + *date_point++ - '0';
	where->minute = count;
	count = 0; date_point++;
	while (*date_point <= '9' && *date_point >= '0')
		count = count * 10 + *date_point++ - '0';
	where->second = count;
	count = 0; date_point++;*/
	sscanf(date_point ,"%d-%d-%d %d:%d:%d" ,&where->year ,&where->month,&where->day,&where->hour,&where->minute ,&where->second);

}

time_t convert(int year, int month, int day)
{
	tm info = { 0 };
	info.tm_year = year - 1900;
	info.tm_mon = month - 1;
	info.tm_mday = day;
	return mktime(&info);
}
void process_data(char *data[], int data_line)
{
	int current_pair = 0, current_day = 0/*,inter_time = 0*/;
	char *vit_id = new char[20] , *scan_tim = new char[20];
	memset(vit_id, 0, 20);
	memset(scan_tim, 0, 20);
	for (int count = 0; count < data_line; count++)
	{
		int count1 = 0; bool flag = 0;
		/*data[count] += 20;
		while (*data[count] <= '9' && *data[count] >= '0')
			count1 = count1 * 10 + *data[count]++ - '0';*/
		
		sscanf(data[count] ,"%s\tflavor%d\t%s",vit_id,&count1,scan_tim);

//		printf(" count1 is %s", scan_tim);
		for (char *count_point = vit_type; *count_point; count_point++)
			if (count1 == *count_point){	flag = 1; break;}
		
			if (count == 0){
				get_date(scan_tim, tran_period); tran_period[0].print_date();
			}
			if (count == data_line - 1) {
//				cout << "there !" << endl;
				get_date(scan_tim, &tran_period[1]); tran_period[1].print_date();
			}

		date_ data_cur;
		get_date(scan_tim, &data_cur);
//		printf("%d \t", ((int)convert(data_cur.year, data_cur.month, data_cur.day) - (int)convert(tran_period[0].year, tran_period[0].month, tran_period[0].day)) / 24 / 3600 - (int)tran_data.size());

		
	for (int vec_count = (int)((convert(data_cur.year, data_cur.month, data_cur.day) - convert(tran_period[0].year, tran_period[0].month, tran_period[0].day)) / 24 / 3600) - (int)tran_data.size(); vec_count > 0; vec_count--)
		{
//			printf("\n interval  is %d\n", ((int)convert(data_cur.year, data_cur.month, data_cur.day) - (int)convert(tran_period[0].year, tran_period[0].month, tran_period[0].day)) / 24 / 3600 );
//			cout << "get there" << endl;
//			printf("\n tran size is %d\n", tran_data.size());
			vitual_pair vit_first;
			vit_first.pair[0] = 0; vit_first.pair[1] = 0;
			data_cur.vit_pair.clear();
			data_cur.vit_pair.push_back(vit_first);
			data_cur.day--;
			tran_data.push_back(data_cur);
			data_cur.day++;
//			printf("\n tran size is %d\n", tran_data.size());
			current_day++; current_pair = 0;
		}


//			printf("flag is %d" ,flag);
		if (flag)
		{
//			printf("vitual type is %d\n", count1);

			if (!tran_data.size())
			{
				vitual_pair vit_int;
				vit_int.pair[0] = count1; vit_int.pair[1] = 1;
//				cout << "get there";
				data_cur.vit_pair.clear();
				data_cur.vit_pair.push_back(vit_int);
				tran_data.push_back(data_cur);
//				cout << (int)tran_data[0].day << (int)tran_data[0].month << tran_data[0].vit_pair[0].pair[0] << tran_data[0].vit_pair[0].pair[1];
			}
			else if (tran_data[current_day].day == data_cur.day && tran_data[current_day].month == data_cur.month && tran_data[current_day].year == data_cur.year)
			{
			/*	if (tran_data[current_day].vit_pair[0].pair[0] == 0)
				{
					cout << "there why!!" << endl;
					tran_data[current_day].vit_pair[0].pair[0] = count1;
					tran_data[current_day].vit_pair[0].pair[1]++;
					current_pair++;
				}
				else*/ 
				bool flag_num = 0;			//know if have this type before
				int count_pair = 0;			
				while (count_pair <= current_pair) if (tran_data[current_day].vit_pair[count_pair++].pair[0] == count1)
				{
					flag_num = 1; break;
				}
				if (flag_num)  tran_data[current_day].vit_pair[--count_pair].pair[1]++;
				else
				{
					vitual_pair vit_push;
//					printf("get there !!!");
					vit_push.pair[0] = count1; vit_push.pair[1] = 1;
					tran_data[current_day].vit_pair.push_back(vit_push);
					current_pair++;
				}
			}
			else
			{
				vitual_pair vit_int;
				vit_int.pair[0] = count1; vit_int.pair[1] = 1;
				data_cur.vit_pair.clear();
				data_cur.vit_pair.push_back(vit_int);
//				cout << "happy new day" << endl;
			//	tran_data.push_back(data_cur);
			//	data_cur.vit_pair[0].pair[0] = count1;
			//	data_cur.vit_pair[0].pair[1] = 1;
				tran_data.push_back(data_cur);
				current_day++; current_pair = 0;
			}
		}
	}
	printf("\ntotal size is %d\n",tran_data.size());
	/*
	for (vector<date_>::iterator iter = tran_data.begin(); iter != tran_data.end(); iter++)
	{
		cout << "date is " << (int)iter->month << (int)iter->day << endl;
		for (vector<vitual_pair>::iterator iter2 = iter->vit_pair.begin(); iter2 != iter->vit_pair.end(); iter2++)
			cout << "finally vitual type is " << iter2->pair[0] << "vitual num is " << iter2->pair[1] << endl;
	}
*/
}
void trans_matrix(void)
{
	flavor_and_day = new int*[vitual];
	for (int count = 0; count < vitual; count++)		flavor_and_day[count] = new int[tran_data.size()];
//	cout << "there get";
	for (int count = 0; count < tran_data.size(); count++)
	if (tran_data[count].vit_pair[0].pair[0] == 0) 
		for (int count2 = 0; count2 < vitual; count2++)
			flavor_and_day[count2][count] = 0;
	else
		for (int count2 = 0; count2 < vitual; count2++)
		{
			int count3 = 0; bool flag = 1;
			while (tran_data[count].vit_pair[count3++].pair[0] != vit_type[count2]) if (count3 >= tran_data[count].vit_pair.size()){	flag = 0; break;}
			if (flag)			flavor_and_day[count2][count] = tran_data[count].vit_pair[count3 - 1].pair[1];
			else				flavor_and_day[count2][count] = 0;
		}

	for (int count = 1; count < tran_data.size(); count++)
		for (int count2 = 0; count2 < vitual; count2++)
				flavor_and_day[count2][count] += flavor_and_day[count2][count - 1];
			
//	cout << "there ok"; //debug : print flavor_and_day 
/*
	for (int count2 = 0; count2 < vitual; count2++) 
	{
		for (int count = 0; count < tran_data.size(); count++)
		{
			cout << flavor_and_day[count2][count] << ' ';
		}
		cout << endl;
	}
*/
}


void printf_result(char *file)
{
	phy_type = new int[phy_num];
	phy_need = new int*[phy_num];
	for (int count = 0; count < phy_num; count++)	phy_need[count] = new int[phy_type[count] * 2];
}

void predict_server(char * info[MAX_INFO_NUM], char * data[MAX_DATA_NUM], int data_num, char * filename)
{
	int count = 0;

	cout << sscanf(info[0] ,"%d %d %d" ,&physical.cpu,&physical.mem,&physical.driver);
	sscanf(info[2] ,"%d" ,&vitual);

	cout << "physical is " << physical.cpu << physical.mem << (int)vitual << endl;

	vit_type = new char[vitual + 1];
	num_predict = new int[vitual + 1];
	memset(vit_type, 0, vitual + 1);
	memset(num_predict, 0, (vitual + 1) * 4);

	for (char count1 = 0; count1 < vitual; count1++)
	{
	/*	info[3 + count1] += 6;
		while (*info[3 + count1] <= '9' && *info[3 + count1] >= '0')
			count = count * 10 + *(info[3 + count1]++) - '0';
		vit_type[count1] = count; count = 0;
		*/
		sscanf(info[count1 + 3] ,"flavor%d" ,&vit_type[count1]);
		printf("vit_type%d is %d",count1 ,vit_type[count1]);
	}

	if (strstr(info[vitual + 4], "CPU")) bias = CPU;
	else bias = MEM;

	get_date(info[vitual + 6], period);
	get_date(info[vitual + 7], &period[1]);

	period[0].print_date();
	period[1].print_date();

	process_data(data ,data_num);
	trans_matrix();
	printf("predict time is %d",day_predict = (int)((convert(period[1].year, period[1].month, period[1].day) - convert(period[0].year, period[0].month, period[0].day))/24/3600));
	printf("tran data time is %d\n", day_tran = (int)((convert(tran_period[1].year, tran_period[1].month, tran_period[1].day) - convert(tran_period[0].year, tran_period[0].month, tran_period[0].day)) / 24 / 3600 + 1));
	//printf_result(filename);
	
	flavor_predict();
	var_int();
	cout<<"�Ѵ�var_int�����˳�"<<endl;
	deploy_method();
	finally_deploy(filename);
		for (i = 0; i<vitual; i++) {
		cout<<"vitual number is "<<virtual_deploy[i][0]<<endl;
	}
}
