
#include "SpeedControl.h"
namespace roadcheck {
		
	SpeedControl::SpeedControl(ros::NodeHandle n) {
		//Parametry pro analizu cestuy
		int frame_length = FRAME_LENGTH_DEF;
		if (n.hasParam(PARAM_LENGTH_PATH)) {
			n.getParam(PARAM_LENGTH_PATH, frame_length);
		}
		double g = G;
		if (n.hasParam(PARAM_G_PATH)) {
			n.getParam(PARAM_G_PATH, g);
		}
		double gt = G/20.0;
		if (n.hasParam(PARAM_G_TH_PATH)) {
			n.getParam(PARAM_G_TH_PATH, gt);
		}
		int frame_length_road = FRAME_LENGTH_ROAD_DEF;
		if (n.hasParam(PARAM_LENGTH_ROAD_PATH)) {
			n.getParam(PARAM_LENGTH_ROAD_PATH, frame_length_road);
		}

		//nastaveni maximalnich rychlosti
		this->max_speed_road = MAX_SPEED_DEF;
		if (n.hasParam(PARAM_MAX_SPEED_ROAD_PATH)) {
			n.getParam(PARAM_MAX_SPEED_ROAD_PATH, this->max_speed_road);
		}

		this->a_acc = 0.5;
		if (n.hasParam(PARAM_A_ACC)) {
			n.getParam(PARAM_A_ACC, this->a_acc);
		}

		this->max_acc = 2.5;
		if (n.hasParam(PARAM_MAX_ACC)) {
			n.getParam(PARAM_MAX_ACC, this->max_acc);
		}
		
		//koeficienty exponencialni funkci
		this->a_speed_road = 0.5;
		if (n.hasParam(PARAM_A_SPEED_ROAD_PATH)) {
			n.getParam(PARAM_A_SPEED_ROAD_PATH, this->a_speed_road);
		}
		this->a_speed_ax = 0.5;
		if (n.hasParam(PARAM_A_SPEED_AX_PATH)) {
			n.getParam(PARAM_A_SPEED_AX_PATH, this->a_speed_ax);
		}
		this->a_speed_ay = 0.5;
		if (n.hasParam(PARAM_A_SPEED_AY_PATH)) {
			n.getParam(PARAM_A_SPEED_AY_PATH, this->a_speed_ay);
		}
		
		this->automat = 0.5;
		if (n.hasParam(PARAM_AUTOMAT)) {
			n.getParam(PPARAM_AUTOMAT, this->automat);
		}

		rc = NULL;
		rc = new RoadCheck(frame_length, g, gt, frame_length_road);

		vel_pub = n.advertise<geometry_msgs::Twist>(CMD_PUB, 1000);
		// kontrolni vypis nastavenych parametru
		printf("SET PARAMETERS:\n");
		printf("frame_length_angle:%d\nframe_length_road:%d\ng:%f\ngth:%f\n", frame_length, frame_length_road, g,gt);

		printf("speed road: %f * (%f)^rc\n", max_speed_road, a_speed_road);
		printf("speed ax: %f * (%f)^acx\n", max_speed_road, a_speed_ax);
		printf("speed ay: %f * (%f)^acy\n", max_speed_road, a_speed_ay);
		printf("acc : %f * (%f)^acy\n", max_acc, a_acc);
		printf("\n\n");
	}

	void SpeedControl::callback(const sensor_msgs::ImuConstPtr &msg) {
		float x = msg->linear_acceleration.x;
		float y = msg->linear_acceleration.y;
		float z = msg->linear_acceleration.z;
		this->roadc = rc->addSample(x, y, z);
	   
		if (rc->countAngles()) {
		   this->anglex = rc->getAngleX();
		   this->angley = rc->getAngleY();
		}
		//printf("%f\n",this->roadc);
		if(this->automat==0){
	   // setMaxSpeed(roadc,fabs(msg->orientation.x), fabs(msg->orientation.y)); 
		setMaxSpeed(roadc,anglex, angley); 
		}
	}


	void SpeedControl::setMaxSpeed(float road_coef, float anglex, float angley) {
		
		static double old_speed = 0;
		static double old_acc = 0;

	   
		double speed_r = pow(a_speed_road, road_coef) * max_speed_road;
		double speed_ax = pow(a_speed_ax, anglex/(PI/4)) * max_speed_road;
		double speed_ay = pow(a_speed_ay, angley/(PI/4)) * max_speed_road;
		double speed = min(speed_r, speed_ax, speed_ay);
		speed = round(speed*100.0)/100.0;
		
		double acc = pow(a_acc, anglex/(PI/4)) * max_acc;

		//if(old_speed == speed) return;
		//dynamicka zmena rychlosti
		if(old_speed != speed){
		   old_speed=speed;
		
		  //generovani prikazu pro zmenu maximalni rychlosti
		   string ex;
		   ex.clear();
		   ex.append(SPEED_SET_TEXT);
		   std::stringstream s;
		   s << speed;
		   ex.append(s.str().c_str());
		   system(ex.c_str());
		   std::cout << ex << std::endl;
		}
		 //dynamicka zmena zruchleni
		 if(old_acc != acc){
		   old_acc=acc;
		   string ex2;
		   ex2.clear();
		   ex2.append(ACCELERATE_SET_TEXT);
		   std::stringstream s2;
		   s2 << acc;
		   ex2.append(s2.str().c_str());
		  system(ex2.c_str());
		   std::cout << ex2 << std::endl;
		 }
	}

	/**
	 * 
	 * @param msg Zprava z topicku o rychlosti
	 */
	void SpeedControl::callbackManual(const geometry_msgs::TwistConstPtr &msg) {
		double max_speed = 0;
		max_speed = msg->linear.x;
		//vypocet rychlosti
		double speed_r = pow(a_speed_road,this->roadc) * max_speed;
		double speed_ax = pow(a_speed_ax, this->anglex/(PI/4)) * max_speed;
		double speed_ay = pow(a_speed_ay, this->angley/(PI/4)) * max_speed;
		double speed = min(speed_r, speed_ax, speed_ay); // vyber nejmensi
		//speed = round(speed*100.0)/100.0;
		//printf("%lf, %lf, %lf\n", max_speed,roadc,speed);
		geometry_msgs::Twist msg_new; //tvorba zpravy o zmene rychlosti
		msg_new.linear.x=speed;
		msg_new.linear.y=msg->linear.y;
		msg_new.linear.z=msg->linear.z;

		msg_new.angular.x=msg->angular.x;
		msg_new.angular.y=msg->angular.y;
		msg_new.angular.z=msg->angular.z;
	  
	   vel_pub.publish(msg_new);
	   ros::spinOnce();
	}


	double SpeedControl::min(double s1, double s2, double s3) {
		double s = s1;
		if (s > s2) {
			s = s2;
		}
		if (s > s3) {
			s = s3;
		}
		return s;
	}
}
