#pragma once
#ifndef METHOD_MC_H
#define METHOD_MC_H

#include "Spirit_Defines.h"
#include <engine/Method_Solver.hpp>
#include <data/Spin_System.hpp>
// #include <data/Parameters_Method_MC.hpp>

#include <vector>

namespace Engine
{
    /*
        The Monte Carlo method
    */
    class Method_MC : public Method
    {
    public:
        // Constructor
        Method_MC(std::shared_ptr<Data::Spin_System> system, int idx_img, int idx_chain);

        // Method name as string
        std::string Name() override;
        
    private:
        // Solver_Iteration represents one iteration of a certain Solver
        void Iteration() override;

        // Metropolis iteration with adaptive cone radius
        void Metropolis(const vectorfield & spins_old, const vectorfield & spins_displaced,
                        vectorfield & spins_new, int & n_rejected, scalar Temperature, scalar radius);

        // Check if the Forces are converged
        bool Converged() override;

        // Save the current Step's Data: spins and energy
        void Save_Current(std::string starttime, int iteration, bool initial=false, bool final=false) override;
        // A hook into the Optimizer before an Iteration
        void Hook_Pre_Iteration() override;
        // A hook into the Optimizer after an Iteration
        void Hook_Post_Iteration() override;

        // Sets iteration_allowed to false for the corresponding method
        void Initialize() override;
        // Sets iteration_allowed to false for the corresponding method
        void Finalize() override;

		// Log message blocks
		void Message_Start() override;
		void Message_Step() override;
		void Message_End() override;



		std::shared_ptr<Data::Parameters_Method_MC> parameters_mc;

		// Cosine of current cone angle
		scalar cos_cone_angle;
		int n_rejected;
		scalar acceptance_ratio_current;
    };
}

#endif