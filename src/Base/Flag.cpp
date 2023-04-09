#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <math.h>
#include <vector>
#include <iostream>


/* Some physics constants */
#define DAMPING 0.01 // how much to damp the Flag simulation each frame
#define TIME_STEPSIZE2 1 // how large time step each particle takes each frame
#define CONSTRAINT_ITERATIONS 15 // how many iterations of constraint satisfaction each frame (more is rigid, less is soft)
#define MASS 1 // mass of one particle

class Vec3
{	
public:
	float f[3];

	Vec3(float x, float y, float z)
	{
		f[0] =x;
		f[1] =y;
		f[2] =z;
	}

	Vec3() {}

	float length()
	{
		return sqrt(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);
	}

	Vec3 normalized()
	{
		float l = length();
		return Vec3(f[0]/l,f[1]/l,f[2]/l);
	}

	void operator+= (const Vec3 &v)
	{
		f[0]+=v.f[0];
		f[1]+=v.f[1];
		f[2]+=v.f[2];
	}

	Vec3 operator/ (const float &a)
	{
		return Vec3(f[0]/a,f[1]/a,f[2]/a);
	}

	Vec3 operator- (const Vec3 &v)
	{
		return Vec3(f[0]-v.f[0],f[1]-v.f[1],f[2]-v.f[2]);
	}

	Vec3 operator+ (const Vec3 &v)
	{
		return Vec3(f[0]+v.f[0],f[1]+v.f[1],f[2]+v.f[2]);
	}

	Vec3 operator* (const float &a)
	{
		return Vec3(f[0]*a,f[1]*a,f[2]*a);
	}

	Vec3 operator-()
	{
		return Vec3(-f[0],-f[1],-f[2]);
	}

	Vec3 cross(const Vec3 &v)
	{
		return Vec3(f[1]*v.f[2] - f[2]*v.f[1], f[2]*v.f[0] - f[0]*v.f[2], f[0]*v.f[1] - f[1]*v.f[0]);
	}

	float dot(const Vec3 &v)
	{
		return f[0]*v.f[0] + f[1]*v.f[1] + f[2]*v.f[2];
	}

};

/* The particle class represents a particle of mass that can move around in 3D space*/
class Particle
{
private:
	bool movable; // can the particle move or not ? used to pin parts of the Flag

	float mass; // the mass of the particle (is always 1 in this example)
	Vec3 old_pos; // the position of the particle in the previous time step, used as part of the verlet numerical integration scheme
	Vec3 acceleration; // a vector representing the current acceleration of the particle
	Vec3 accumulated_normal; // an accumulated normal (i.e. non normalized), used for OpenGL soft shading

public:
	Vec3 pos; // the current position of the particle in 3D space

	Particle(Vec3 pos,float mass) : pos(pos), old_pos(pos),acceleration(Vec3(0,0,0)), mass(MASS), movable(true), accumulated_normal(Vec3(0,0,0)){}
	Particle(){}

	void addForce(Vec3 f)
	{
		acceleration += f/mass;
	}

	void timeStep()
	{
		if(movable)
		{
			Vec3 temp = pos;
			pos = pos + (pos-old_pos) * (1.0 - DAMPING) + acceleration*TIME_STEPSIZE2;
			old_pos = temp;
			acceleration = Vec3(0,0,0); // acceleration is reset since it HAS been translated into a change in position (and implicitely into velocity)	
		}
	}

	Vec3& getPos() {return pos;}

	void resetAcceleration() {acceleration = Vec3(0,0,0);}

	void offsetPos(const Vec3 v) { if(movable) pos += v;}

	void makeUnmovable() {movable = false;}

	void addToNormal(Vec3 normal)
	{
		accumulated_normal += normal.normalized();
	}

	Vec3& getNormal() { return accumulated_normal;} // notice, the normal is not unit length

	void resetNormal() {accumulated_normal = Vec3(0,0,0);}
};

class Constraint
{
private:
	float rest_distance; // the length between particle p1 and p2 in rest configuration

public:
	Particle *p1, *p2; // the two particles that are connected through this constraint

	Constraint(Particle *p1, Particle *p2) :  p1(p1),p2(p2)
	{
		Vec3 vec = p1->getPos()-p2->getPos();
		rest_distance = vec.length();
	}

	/* This is one of the important methods, where a single constraint between two particles p1 and p2 is solved
	the spring model is very simplified we don't take into account elasticity value or fluid friction(air) */
	void satisfyConstraint()
	{
		Vec3 p1_to_p2 = p2->getPos()-p1->getPos(); // vector from p1 to p2
		float current_distance = p1_to_p2.length(); // current distance between p1 and p2
		Vec3 correctionVector = p1_to_p2*(1 - rest_distance/current_distance); // The offset vector that could moves p1 into a distance of rest_distance to p2
		Vec3 correctionVectorHalf = correctionVector*0.5; // Lets make it half that length, so that we can move BOTH p1 and p2.
		p1->offsetPos(correctionVectorHalf); // correctionVectorHalf is pointing from p1 to p2, so the length should move p1 half the length needed to satisfy the constraint.
		p2->offsetPos(-correctionVectorHalf); // we must move p2 the negative direction of correctionVectorHalf since it points from p2 to p1, and not p1 to p2.	
	}
};

class Flag
{
private:

	int num_particles_width; 
	int num_particles_height; 
	// total number of particles is num_particles_width*num_particles_height

	std::vector<Particle> particles; // all particles that are part of this Flag
	std::vector<Constraint> constraints; // alle constraints between particles as part of this Flag

	Particle* getParticle(int x, int y) {return &particles[y*num_particles_width + x];}
	void makeConstraint(Particle *p1, Particle *p2) {constraints.push_back(Constraint(p1,p2));}

	/* A private method used for flag rendering to retrieve the  
	normal vector of the triangle defined by the position of the particles p1, p2, and p3.
	The magnitude of the normal vector = the area of the parallelogram (P1P2,P1P3)
	*/
	Vec3 calcTriangleNormal(Particle *p1,Particle *p2,Particle *p3)
	{
		Vec3 pos1 = p1->getPos();
		Vec3 pos2 = p2->getPos();
		Vec3 pos3 = p3->getPos();

		Vec3 v1 = pos2-pos1;
		Vec3 v2 = pos3-pos1;

 		return v1.cross(v2);
	}

	void AddTriangle(Particle *p1, Particle *p2, Particle *p3)
	{
        flag_vertices.push_back(p1->getPos().f[0]);
        flag_vertices.push_back(p1->getPos().f[1]);
        flag_vertices.push_back(p1->getPos().f[2]);
        
        flag_vertices.push_back(p1->getNormal().normalized().f[0]);
        flag_vertices.push_back(p1->getNormal().normalized().f[1]);
        flag_vertices.push_back(p1->getNormal().normalized().f[2]);

        flag_vertices.push_back(p2->getPos().f[0]);
        flag_vertices.push_back(p2->getPos().f[1]);
        flag_vertices.push_back(p2->getPos().f[2]);

        flag_vertices.push_back(p2->getNormal().normalized().f[0]);
        flag_vertices.push_back(p2->getNormal().normalized().f[1]);
        flag_vertices.push_back(p2->getNormal().normalized().f[2]);

        flag_vertices.push_back(p3->getPos().f[0]);
        flag_vertices.push_back(p3->getPos().f[1]);
        flag_vertices.push_back(p3->getPos().f[2]);

        flag_vertices.push_back(p3->getNormal().normalized().f[0]);
        flag_vertices.push_back(p3->getNormal().normalized().f[1]);
        flag_vertices.push_back(p3->getNormal().normalized().f[2]);
	}
    GLuint VAO,VBO;
    std::vector<float> flag_vertices;
public:

	/* This is a important constructor for the entire system of particles and constraints*/
	Flag(float width, float height, int num_particles_width, int num_particles_height) : num_particles_width(num_particles_width), num_particles_height(num_particles_height)
	{
		particles.resize(num_particles_width*num_particles_height);
		float mass_particle = MASS/num_particles_width*num_particles_height;

		for(int x=0; x<num_particles_width; x++)
		{
			for(int y=0; y<num_particles_height; y++)
			{
				Vec3 pos = Vec3(width * (x/(float)num_particles_width),
								height * (y/(float)num_particles_height),
								0);
				particles[y*num_particles_width+x]= Particle(pos,0.0001); // insert particle in column x at y'th row
			}
		}

		// Connecting immediate neighbor particles with constraints (distance 1 and sqrt(2) in the grid)
		for(int x=0; x<num_particles_width; x++)
		{
			for(int y=0; y<num_particles_height; y++)
			{
				if (x<num_particles_width-1) makeConstraint(getParticle(x,y),getParticle(x+1,y));
				if (y<num_particles_height-1) makeConstraint(getParticle(x,y),getParticle(x,y+1));
				if (x<num_particles_width-1 && y<num_particles_height-1) makeConstraint(getParticle(x,y),getParticle(x+1,y+1));
				if (x<num_particles_width-1 && y<num_particles_height-1) makeConstraint(getParticle(x+1,y),getParticle(x,y+1));
			}
		}

		// Connecting secondary neighbors with constraints (distance 2 and sqrt(4) in the grid)
		for(int x=0; x<num_particles_width; x++)
		{
			for(int y=0; y<num_particles_height; y++)
			{
				if (x<num_particles_width-2) makeConstraint(getParticle(x,y),getParticle(x+2,y));
				if (y<num_particles_height-2) makeConstraint(getParticle(x,y),getParticle(x,y+2));
				if (x<num_particles_width-2 && y<num_particles_height-2) makeConstraint(getParticle(x,y),getParticle(x+2,y+2));
				if (x<num_particles_width-2 && y<num_particles_height-2) makeConstraint(getParticle(x+2,y),getParticle(x,y+2));			
            }
		}

        for(int j=0;j<num_particles_width; j++)
        {
            getParticle(0 ,j)->makeUnmovable(); 
        }
	}

	/* drawing the Flag as a smooth shaded (and colored according to column) OpenGL triangular mesh
	Called from the display() method
	The Flag is seen as consisting of triangles for four particles in the grid as follows:

	(x+1,y) *--* (x+1,y+1)
	        | /|
	        |/ |
	(x,y)   *--* (x,y+1)

	*/
	void render()
	{
		// reset normals (which where written to last frame)
		std::vector<Particle>::iterator particle;
		for(particle = particles.begin(); particle != particles.end(); particle++)
		{
			(*particle).resetNormal();
		}

		//create smooth per particle normals by adding up all the (hard) triangle normals that each particle is part of
		for(int x = 0; x<num_particles_width-1; x++)
		{
			for(int y=0; y<num_particles_height-1; y++)
			{
				Vec3 normal = calcTriangleNormal(getParticle(x+1,y),getParticle(x,y),getParticle(x,y+1));
				getParticle(x+1,y+1)->addToNormal(normal);
				getParticle(x+1,y)->addToNormal(normal);
				getParticle(x,y)->addToNormal(normal);

				normal = calcTriangleNormal(getParticle(x+1,y+1),getParticle(x+1,y),getParticle(x,y+1));
				getParticle(x+1,y+1)->addToNormal(normal);
				getParticle(x,y)->addToNormal(normal);
				getParticle(x,y+1)->addToNormal(normal);
			}
		}

		for(int x = 0; x<num_particles_width-1; x++)
		{
			for(int y=0; y<num_particles_height-1; y++)
			{
				AddTriangle(getParticle(x,y),getParticle(x,y+1),getParticle(x+1,y+1));
				AddTriangle(getParticle(x,y),getParticle(x+1,y),getParticle(x+1,y+1));
			}
		}

		// setup VAO
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, flag_vertices.size() * sizeof(float), &flag_vertices[0], GL_STATIC_DRAW);
		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// Normal attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, num_particles_width * num_particles_height * 2 * 3);
        glBindVertexArray(0);
		
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);

		flag_vertices.clear();
	}

	/* this is an important methods where the time is progressed one time step for the entire Flag.
	This includes calling satisfyConstraint() for every constraint, and calling timeStep() for all particles
	*/
	void timeStep()
	{
		std::vector<Constraint>::iterator constraint;
		for(int i=0; i<CONSTRAINT_ITERATIONS; i++) // iterate over all constraints several times
		{
			for(constraint = constraints.begin(); constraint != constraints.end(); constraint++ )
			{
				(*constraint).satisfyConstraint(); // satisfy constraint.
			}
		}
		std::vector<Particle>::iterator particle;
		for(particle = particles.begin(); particle != particles.end(); particle++)
		{
			particle->timeStep(); // calculate the position of each particle at the next time step.
		}
	}

	/* used to add gravity (or any other arbitrary vector) to all particles*/
	void addForce(const Vec3 force)
	{
		std::vector<Particle>::iterator particle;
		for(particle = particles.begin(); particle != particles.end(); particle++)
		{
			particle->addForce(force); // add the forces to each particle
		}

	}

	/* used to add wind forces to all particles, is added for each triangle since the final force is proportional to the triangle area as seen from the wind direction*/
	void addwindForce(const Vec3 direction)
	{
		for(int x = 0; x<num_particles_width-1; x++)
		{
			for(int y=0; y<num_particles_height-1; y++)
			{
				Particle* p1 = getParticle(x,y);
				Particle* p2 = getParticle(x+1,y);
				Particle* p3 = getParticle(x,y+1);
				Particle* p4 = getParticle(x+1,y+1);
				Vec3 normal = calcTriangleNormal(p2,p1,p3);
				Vec3 d = normal.normalized();
				Vec3 force = normal*(d.dot(direction));
				p1->addForce(force);
				p2->addForce(force);
				p3->addForce(force);


				normal = calcTriangleNormal(p4,p2,p3);
				d = normal.normalized();
				force = normal*(d.dot(direction));
				p2->addForce(force);
				p3->addForce(force);
				p4->addForce(force);
			}
		}
	}

};
