/*
 Copyright 2016-2020 Intel Corporation
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
     http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char *argv[]) {
    const size_t count = 10 * 1024 * 1024;

    int i = 0;
    int j = 0;
    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    queue q;
    if (!create_sycl_queue(argc, argv, rank, q)) {
        return -1;
    }

    /* create kvs */
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }

    /* create communicator */
    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());
    auto comm = ccl::create_communicator(size, rank, dev, ctx, kvs);

    /* create stream */
    auto stream = ccl::create_stream(q);

    /* create buffers */
    buffer<int> send_buf(count);
    buffer<int> expected_buf(count * size);
    buffer<int> recv_buf(size * count);

    vector<size_t> recv_counts(size, count);

    {
        /* open buffers and initialize them on the host side */
        host_accessor send_buf_acc(send_buf, write_only);
        host_accessor recv_buf_acc(recv_buf, write_only);
        host_accessor expected_acc_buf(expected_buf, write_only);

        for (i = 0; i < count; i++) {
            send_buf_acc[i] = rank;
        }
        for (i = 0; i < count * size; i++) {
            recv_buf_acc[i] = -1;
        }
        for (i = 0; i < size; i++) {
            for (j = 0; j < count; j++) {
                expected_acc_buf[i * count + j] = i + 1;
            }
        }
    }

    /* open send_buf and modify it on the device side */
    auto e = q.submit([&](auto &h) {
        accessor send_buf_acc(send_buf, h, write_only);
        h.parallel_for(count, [=](auto id) {
            send_buf_acc[id] += 1;
        });
    });

    /* create dependency vector */
    vector<ccl::event> events;
    //events.push_back(ccl::create_event(e));

    if (!handle_exception(q))
        return -1;

    /* invoke allagtherv */
    auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();
    ccl::allgatherv(send_buf, count, recv_buf, recv_counts, comm, stream, attr, events).wait();

    /* open recv_buf and check its correctness on the device side */
    q.submit([&](auto &h) {
        accessor recv_buf_acc(recv_buf, h, write_only);
        accessor expected_buf_acc(expected_buf, h, read_only);
        h.parallel_for(size * count, [=](auto id) {
            if (recv_buf_acc[id] != expected_buf_acc[id]) {
                recv_buf_acc[id] = -1;
            }
        });
    });

    if (!handle_exception(q))
        return -1;

    /* print out the result of the test on the host side */
    {
        host_accessor recv_buf_acc(recv_buf, read_only);
        for (i = 0; i < size * count; i++) {
            if (recv_buf_acc[i] == -1) {
                cout << "FAILED\n";
                break;
            }
        }
        if (i == size * count) {
            cout << "PASSED\n";
        }
    }

    return 0;
}
