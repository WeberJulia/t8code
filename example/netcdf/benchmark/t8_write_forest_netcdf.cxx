/*
    This file is part of t8code.
    t8code is a C library to manage a collection (a forest) of multiple
    connected adaptive space-trees of general element types in parallel.

    Copyright (C) 2015 the developers

    t8code is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    t8code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with t8code; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <t8.h>
#if T8_WITH_NETCDF
	#include <netcdf.h>
#else
	/* Normally defined in 'netcdf.h' */
	#define NC_CHUNKED 1
	#define NC_CONTIGUOUS 1
#endif
#if T8_WITH_NETCDF_PAR
	#include <netcdf_par.h>
#else
	/* Normally defined in 'netcdf_par.h' */
	#define NC_INDEPENDENT 0
	#define NC_COLLECTIVE 1
#endif
#include <t8_cmesh.h>
#include <t8_cmesh/t8_cmesh_examples.h>
#include <t8_eclass.h>
#include <t8_forest.h>
#include <t8_forest/t8_forest_iterate.h>
#include <t8_forest_netcdf.h>
#include <t8_netcdf.h>
#include <t8_schemes/t8_default/t8_default_cxx.hxx>
#include <t8_vec.h>

#include <cmath>
#include <random>
#include <stdexcept>
#include <string_view>
#include <vector>

/* In this example is the use of the netcdf feature exemplary displayed.
 * We show how to write out a forest in the netCDF format and how to create
 * additional (integer/double) netCDF variables which hold element data. There
 * are mainly two funcitons implemented in 'src/t8_forest_netcdf.cxx' which
 * allow the creation of netCDF file containing the data for a forest in the
 * style of the UGRID conventions. The first function is:
 * 't8_forest_write_netcdf_ext()'; it allows to choose which variable storage
 * and access scheme should be used (e.g. {NC_CONTIGUOUS;
 * NC_CHUNKED}x{NC_INDEPENDENT; NC_COLLECTIVE}). The second function is:
 * 't8_forest_write_netcdf()' uses default values (NC_CONTIGUOUS,
 * NC_INDEPENDENT). If the extended function is used and NC_CHUNKED is chosen:
 * Curently, the chunksize which is chosen is the netCDF default (this means ->
 * nc_def_var_chunking(..., NULL) receives a NULL-pointer as the 'size_t*
 * chunksizesp' parameter)
 */

struct adapt_user_data {
	double additionally_refined_ratio;
	std::mt19937_64 rne;
};

int t8_example_netcdf_adapt_fn(
	t8_forest_t forest, t8_forest_t forest_from, t8_locidx_t which_tree,
	t8_locidx_t lelement_id, t8_eclass_scheme_c* ts, const int is_family,
	const int num_elements, t8_element_t* elements[]
) {
	auto& adapt_data =
		*static_cast<adapt_user_data*>(t8_forest_get_user_data(forest));

	std::bernoulli_distribution should_refine{
		adapt_data.additionally_refined_ratio};
	return should_refine(adapt_data.rne) ? 1 : 0;
}

struct RefinementConfig {
	double additionally_refined_ratio;
	int initial;
};

struct Config {
	RefinementConfig refinement;
	int netcdf_var_storage_mode;
	int netcdf_mpi_access = NC_COLLECTIVE;
	int fill_mode;
	int cmode;
	bool multifile_mode = false;
};

/** Function that times the duration of writing out the netCDF File, given a
 * specific variable storage and access pattern \param [in] forest The forest to
 * save in a netCDF file (using UGRID conventions). \param [in] comm The MPI
 * communicator to use. \param [in] netcdf_var_storage_mode Choose if chunked or
 * contiguous storage should be used (possible Options: NC_CONTIGUOUS,
 * NC_CHUNCKED). \param [in] netcdf_var_mpi_access Choose if the netCDF write
 * operations should be performed independently or collectively by the MPI ranks
 * (possible Options: NC_INDEPENDENT, NC_COLLECTIVE). \param [in] title Hold the
 * title of the netCDF file which is stored inside the netCDF file as a global
 * attribute. \param [in] num_additonal_vars The number of additional
 * user-variables to write out. \param [in] ext_vars A pointer to an array which
 * holds \a num_additional_vars which should be written out in addition to the
 * 'forest NetCDF variables' \note It is assumed that each user-variable in \a
 * ext_vars holds one value for each element in the mesh/forest. If no
 * additional variables should be written in the netCDF file, set \a
 * num_additional_vars equal to zero and pass a NULL-pointer as \a ext_vars.
 */
static void t8_example_time_netcdf_writing_operation(
	t8_forest_t forest, sc_MPI_Comm comm, Config config, const char* title
) {
	sc_MPI_Barrier(comm);
	double start_time = sc_MPI_Wtime();

	/* Write out the forest in netCDF format using the extended function which
	 * allows to set a specific variable storage and access pattern. */
	t8_forest_write_netcdf_ext(
		forest, title, "Performance Test: uniformly refined Forest", 3, 0,
		nullptr, comm, config.netcdf_var_storage_mode, nullptr,
		config.netcdf_mpi_access, config.fill_mode, config.cmode,
		config.multifile_mode
	);

	sc_MPI_Barrier(comm);
	double end_time = sc_MPI_Wtime();
	double duration = end_time - start_time;
	double global;
	auto retval = sc_MPI_Reduce(
		&duration, &global, 1, sc_MPI_DOUBLE, sc_MPI_MAX, 0, comm
	);
	SC_CHECK_MPI(retval);

	t8_global_productionf(
		"The time elapsed to write the netCDF-4 File is: %f\n\n", global
	);
}

auto elements_needed_for_bytes(long long bytes) { return bytes / 268.0; }

auto config_for_bytes(long long bytes) {
	const auto nMesh3D_vol = elements_needed_for_bytes(bytes);
	RefinementConfig config;
	config.initial = std::max(std::floor(std::log2(nMesh3D_vol / 16) / 3), 0.0);
	config.additionally_refined_ratio =
		nMesh3D_vol / (7 * 16 * std::pow(8.0, config.initial)) - 1 / 7.0;
	return config;
}

Config parse_args(int argc, char** argv) {
	std::vector<std::string_view> args{argv + 1, argv + argc};
	Config result;

	const auto total_bytes = std::stoll(std::string{args.at(0)});
	result.refinement = config_for_bytes(total_bytes);

	if (args.at(1) == "NC_FILL") {
		result.fill_mode = NC_FILL;
	} else if (args.at(1) == "NC_NOFILL") {
		result.fill_mode = NC_NOFILL;
	} else {
		throw std::runtime_error{"fill must be one of NC_FILL and NC_NOFILL"};
	}
	if (args.at(2) == "classic") {
		result.cmode = NC_CLASSIC_MODEL | NC_64BIT_DATA;
	} else if (args.at(2) == "netcdf4_hdf5") {
		result.cmode = NC_NETCDF4;
	} else {
		throw std::runtime_error{
			"cmode must be one of \"classic\" and \"netcdf4_hdf5\""};
	}
	if (args.at(3) == "NC_CONTIGUOUS") {
		result.netcdf_var_storage_mode = NC_CONTIGUOUS;
	} else if (args.at(3) == "NC_CHUNKED") {
		result.netcdf_var_storage_mode = NC_CHUNKED;
	} else {
		throw std::runtime_error{
			"storage mode must be one of NC_CONTIGUOUS and NC_CHUNKED"};
	}
	if (args.at(4) == "NC_INDEPENDENT") {
		result.netcdf_mpi_access = NC_INDEPENDENT;
	} else if (args.at(4) == "NC_COLLECTIVE") {
		result.netcdf_mpi_access = NC_COLLECTIVE;
	} else if (args.at(4) == "--multifile") {
		result.multifile_mode = true;
	} else {
		throw std::runtime_error{"this argument is either mpi access "
		                         "(NC_COLLECTIVE or NC_INDEPENDENT), "
		                         "or --multifile"};
	}

	return result;
}

auto adapt_forest(t8_forest_t forest, double additionally_refined_ratio) {
	adapt_user_data adapt_data{
		.additionally_refined_ratio = additionally_refined_ratio};
	return t8_forest_new_adapt(
		forest, t8_example_netcdf_adapt_fn, 0, 0, &adapt_data
	);
}

void execute_benchmark(sc_MPI_Comm comm, Config config) {
	int mpirank;
	{
		int retval = sc_MPI_Comm_rank(comm, &mpirank);
		SC_CHECK_MPI(retval);
	}

	/* Construct a 3D hybrid hypercube as a cmesh */
	t8_cmesh_t cmesh = t8_cmesh_new_hypercube_hybrid(comm, 1, 0);

	/* Build a (partioined) uniform forest */
	t8_forest_t forest = t8_forest_new_uniform(
		cmesh, t8_scheme_new_default_cxx(), config.refinement.initial, 0, comm
	);

	forest = adapt_forest(forest, config.refinement.additionally_refined_ratio);

	t8_productionf(
		"Number of process-local elements: %d\n",
		t8_forest_get_local_num_elements(forest)
	);

	t8_global_productionf(
		"The uniformly refined forest (refinement level = %d) "
		"has %ld global elements.\n",
		config.refinement.initial, t8_forest_get_global_num_elements(forest)
	);

	t8_global_productionf(
		"Variable-Storage: %s, Variable-Access: %s:\n",
		config.netcdf_var_storage_mode == NC_CHUNKED ? "NC_CHUNKED"
													 : "NC_CONTIGUOUS",
		config.netcdf_mpi_access == NC_COLLECTIVE ? "NC_COLLECTIVE"
												  : "NC_INDEPENDENT"
	);
	t8_example_time_netcdf_writing_operation(
		forest, comm, config, "T8_Example_NetCDF_Performance"
	);

	/* Destroy the forest */
	t8_forest_unref(&forest);
}

int main(int argc, char** argv) {
	SC_CHECK_MPI(sc_MPI_Init(&argc, &argv));
	sc_init(sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
	t8_init(SC_LP_PRODUCTION);

	Config config;
	try {
		config = parse_args(argc, argv);
	} catch (const std::exception& e) {
		t8_global_productionf("Could not parse arguments. Reason:\n");
		t8_global_productionf("%s\n", e.what());
		t8_global_productionf(
			R"asdf(Usage: ./t8_write_forest_netcdf <mem_per_node> <fill> <cmode> <storage_mode> <mpi_access_mode>
Usage: ./t8_write_forest_netcdf <mem_per_node> <fill> <cmode> <storage_mode> --multifile
)asdf"
		);
		sc_finalize();
		SC_CHECK_MPI(sc_MPI_Finalize());
		return EXIT_FAILURE;
	}

	execute_benchmark(sc_MPI_COMM_WORLD, config);

	sc_finalize();
	SC_CHECK_MPI(sc_MPI_Finalize());
}
