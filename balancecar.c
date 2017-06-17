/****************************************************************************
   �ǲ����ܿƼ����޹�˾
   ��Ʒ���ƣ�Arduino ����ƽ��С��
   ��Ʒ�ͺţ�BST-ABC ver 1.3
****************************************************************************/


#include <PinChangeInt.h>
#include <MsTimer2.h>
//���ò������̼���ʵ���ٶ�PID����
#include <BalanceCar.h>
#include <KalmanFilter.h>
//I2Cdev��MPU6050��PID_v1�����Ҫ���Ȱ�װ��Arduino ����ļ�����
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"

MPU6050 mpu; //ʵ����һ�� MPU6050 ���󣬶�������Ϊ mpu
BalanceCar balancecar;
KalmanFilter kalmanfilter;
int16_t ax, ay, az, gx, gy, gz;
//TB6612FNG����ģ������ź�
#define IN1M 7
#define IN2M 6
#define IN3M 13
#define IN4M 12
#define PWMA 9
#define PWMB 10
#define STBY 8

#define PinA_left 2  //�ж�0
#define PinA_right 4 //�ж�1



//�����Զ������
int time;
byte inByte; //���ڽ����ֽ�
int num;
double Setpoint;                               //�Ƕ�DIP�趨�㣬���룬���
double Setpoints, Outputs = 0;                         //�ٶ�DIP�趨�㣬���룬���
double kp = 38, ki = 0.0, kd = 0.58;                   //��Ҫ���޸ĵĲ���
double kp_speed = 4.1, ki_speed = 0.1058, kd_speed = 0.0;            // ��Ҫ���޸ĵĲ���
double kp_turn = 28, ki_turn = 0, kd_turn = 0.29;                 //��תPID�趨
//ת��PID����
double setp0 = 0, dpwm = 0, dl = 0; //�Ƕ�ƽ��㣬PWM�������PWM1��PWM2
float value;


//********************angle data*********************//
float Q;
float Angle_ax; //�ɼ��ٶȼ������б�Ƕ�
float Angle_ay;
float K1 = 0.05; // �Լ��ٶȼ�ȡֵ��Ȩ��
float angle0 = 0.00; //��еƽ���
int slong;
//********************angle data*********************//

//***************Kalman_Filter*********************//
float Q_angle = 0.001, Q_gyro = 0.005; //�Ƕ��������Ŷ�,���ٶ��������Ŷ�
float R_angle = 0.5 , C_0 = 1;
float timeChange = 5; //�˲�������ʱ��������
float dt = timeChange * 0.001; //ע�⣺dt��ȡֵΪ�˲�������ʱ��
//***************Kalman_Filter*********************//

//*********************************************
//******************** speed count ************
//*********************************************

volatile long count_right = 0;//ʹ��volatile lon������Ϊ���ⲿ�ж��������ֵ������������ʹ��ʱ��ȷ����ֵ��Ч
volatile long count_left = 0;//ʹ��volatile lon������Ϊ���ⲿ�ж��������ֵ������������ʹ��ʱ��ȷ����ֵ��Ч
int speedcc = 0;

//////////////////////�������/////////////////////////
int lz = 0;
int rz = 0;
int rpluse = 0;
int lpluse = 0;

/////////////////////�������////////////////////////////

//////////////ת����ת����///////////////////////////////
int turncount = 0; //ת�����ʱ�����
float turnoutput = 0;
//////////////ת����ת����///////////////////////////////

//////////////����������///////////////////
int front = 0;//ǰ������
int back = 0;//���˱���
int turnl = 0;//��ת��־
int turnr = 0;//��ת��־
int spinl = 0;//����ת��־
int spinr = 0;//����ת��־
//////////////����������///////////////////

//////////////////�������ٶ�//////////////////

int chaoshengbo = 0;
int tingzhi = 0;
int jishi = 0;

//////////////////�������ٶ�//////////////////


//////////////////////�������///////////////////////
void countpluse()
{

  lz = count_left;
  rz = count_right;
  
  count_left = 0;
  count_right = 0;

  lpluse = lz;
  rpluse = rz;

  if ((balancecar.pwm1 < 0) && (balancecar.pwm2 < 0))                     //С���˶������ж� ����ʱ��PWM�������ѹΪ���� ������Ϊ����
  {
    rpluse = -rpluse;
    lpluse = -lpluse;
  }
  else if ((balancecar.pwm1 > 0) && (balancecar.pwm2 > 0))                 //С���˶������ж� ǰ��ʱ��PWM�������ѹΪ���� ������Ϊ����
  {
    rpluse = rpluse;
    lpluse = lpluse;
  }
  else if ((balancecar.pwm1 < 0) && (balancecar.pwm2 > 0))                 //С���˶������ж� ǰ��ʱ��PWM�������ѹΪ���� ������Ϊ����
  {
    rpluse = rpluse;
    lpluse = -lpluse;
  }
  else if ((balancecar.pwm1 > 0) && (balancecar.pwm2 < 0))               //С���˶������ж� ����ת ��������Ϊ���� ��������Ϊ����
  {
    rpluse = -rpluse;
    lpluse = lpluse;
  }

  //�����ж�
  balancecar.stopr += rpluse;
  balancecar.stopl += lpluse;

  //ÿ5ms�����ж�ʱ������������
  balancecar.pulseright += rpluse;
  balancecar.pulseleft += lpluse;

}
////////////////////�������///////////////////////



//////////////////�Ƕ�PD////////////////////
void angleout()
{
  balancecar.angleoutput = kp * (kalmanfilter.angle + angle0) + kd * kalmanfilter.Gyro_x;//PD �ǶȻ�����
}
//////////////////�Ƕ�PD////////////////////

//////////////////////////////////////////////////////////
//////////////////�ж϶�ʱ 5ms��ʱ�ж�////////////////////
/////////////////////////////////////////////////////////
void inter()
{
  sei();                                           
  countpluse();                                     //��������Ӻ���
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);     //IIC��ȡMPU6050�������� ax ay az gx gy gz
  kalmanfilter.Angletest(ax, ay, az, gx, gy, gz, dt, Q_angle, Q_gyro,R_angle,C_0,K1);                                      //��ȡangle �ǶȺͿ����˲�
  angleout();                                       //�ǶȻ� PD����

  speedcc++;
  if (speedcc >= 8)                                //40ms�����ٶȻ�����
  {
    Outputs = balancecar.speedpiout(kp_speed,ki_speed,kd_speed,front,back,setp0);
    speedcc = 0;
  }
    turncount++;
  if (turncount > 4)                                //40ms������ת����
  {
    turnoutput = balancecar.turnspin(turnl,turnr,spinl,spinr,kp_turn,kd_turn,kalmanfilter.Gyro_z);                                    //��ת�Ӻ���
    turncount = 0;
  }
  balancecar.posture++;
  balancecar.pwma(Outputs,turnoutput,kalmanfilter.angle,kalmanfilter.angle6,turnl,turnr,spinl,spinr,front,back,kalmanfilter.accelz,IN1M,IN2M,IN3M,IN4M,PWMA,PWMB);                                            //С����PWM���
}
//////////////////////////////////////////////////////////
//////////////////�ж϶�ʱ 5ms��ʱ�ж�///////////////////
/////////////////////////////////////////////////////////



// ===    ��ʼ����     ===
void setup() {
  // TB6612FNGN����ģ������źų�ʼ��
  pinMode(IN1M, OUTPUT);                         //���Ƶ��1�ķ���01Ϊ��ת��10Ϊ��ת
  pinMode(IN2M, OUTPUT);
  pinMode(IN3M, OUTPUT);                        //���Ƶ��2�ķ���01Ϊ��ת��10Ϊ��ת
  pinMode(IN4M, OUTPUT);
  pinMode(PWMA, OUTPUT);                        //����PWM
  pinMode(PWMB, OUTPUT);                        //�ҵ��PWM
  pinMode(STBY, OUTPUT);                        //TB6612FNGʹ��


  //��ʼ���������ģ��
  digitalWrite(IN1M, 0);
  digitalWrite(IN2M, 1);
  digitalWrite(IN3M, 1);
  digitalWrite(IN4M, 0);
  digitalWrite(STBY, 1);
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);

  pinMode(PinA_left, INPUT);  //������������
  pinMode(PinA_right, INPUT);

  // ����I2C����
  Wire.begin();                            //���� I2C ��������
  Serial.begin(9600);                       //�������ڣ����ò�����Ϊ 115200
  delay(1500);
  mpu.initialize();                       //��ʼ��MPU6050
  delay(2);
 //5ms��ʱ�ж�����  ʹ��timer2    ע�⣺ʹ��timer2���pin3 pin11��PWM�����Ӱ�죬��ΪPWMʹ�õ��Ƕ�ʱ������ռ�ձȣ�������ʹ��timer��ʱ��Ҫע��鿴��Ӧtimer��pin�ڡ�
  MsTimer2::set(5, inter);
  MsTimer2::start();

}

////////////////////////bluetooth//////////////////////
void kongzhi()
{
  while (Serial.available())                                    //�ȴ���������
    switch (Serial.read())                                      //��ȡ��������
    {
      case 0x01: front = 500;   break;                         //ǰ��
      case 0x02: back = -500;   break;                        //����
      case 0x03: turnl = 1;   break;                          //��ת
      case 0x04: turnr = 1;   break;                          //��ת
      case 0x05: spinl = 1;   break;                       //����ת
      case 0x06: spinr = 1;   break;                       //����ת
      case 0x07: turnl = 0; turnr = 0;  front = 0; back = 0; spinl = 0; spinr = 0;  break;                    //ȷ�������ɿ���Ϊͣ������
      case 0x08: spinl = 0; spinr = 0;  front = 0; back = 0;  turnl = 0; turnr = 0;  break;                  //ȷ�������ɿ���Ϊͣ������
      case 0x09: front = 0; back = 0; turnl = 0; turnr = 0; spinl = 0; spinr = 0; turnoutput = 0; break;       // ȷ�������ɿ���Ϊͣ������
      default: front = 0; back = 0; turnl = 0; turnr = 0; spinl = 0; spinr = 0; turnoutput = 0; break;
    }
}


////////////////////////////////////////turn//////////////////////////////////



// ===       ��ѭ��������       ===
void loop() {

  //��������ѭ����⼰�������� �ⶨС������  ʹ�õ�ƽ�ı�Ƚ���������� ���ӵ��������������֤С���ľ�ȷ�ȡ�
  attachInterrupt(0, Code_left, CHANGE);
  attachPinChangeInterrupt(PinA_right, Code_right, CHANGE);
  //��������
  kongzhi();
}

////////////////////////////////////////pwm///////////////////////////////////



//////////////////////////�����жϼ���/////////////////////////////////////

void Code_left() {

  count_left ++;

} //��������̼���



void Code_right() {

  count_right ++;

} //�Ҳ������̼���

//////////////////////////�����жϼ���/////////////////////////////////////


