// -*- C++ -*-
//===-- scan.pass.cpp -----------------------------------------------------===//
//
// Copyright (C) Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// This file incorporates work covered by the following copyright and permission
// notice:
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
//
//===----------------------------------------------------------------------===//

#include "support/pstl_test_config.h"

#include _PSTL_TEST_HEADER(execution)
#include _PSTL_TEST_HEADER(numeric)

#include "support/utils.h"

#if  !defined(_PSTL_TEST_INCLUSIVE_SCAN) && !defined(_PSTL_TEST_EXCLUSIVE_SCAN)
#define _PSTL_TEST_INCLUSIVE_SCAN
#define _PSTL_TEST_EXCLUSIVE_SCAN
#endif

using namespace TestUtils;

// We provide the no execution policy versions of the exclusive_scan and inclusive_scan due checking correctness result of the versions with execution policies.
//TODO: to add a macro for availability of ver implementations
template <class InputIterator, class OutputIterator, class T>
OutputIterator
exclusive_scan_serial(InputIterator first, InputIterator last, OutputIterator result, T init)
{
    for (; first != last; ++first, ++result)
    {
        *result = init;
        init = init + *first;
    }
    return result;
}

template <class InputIterator, class OutputIterator, class T, class BinaryOperation>
OutputIterator
exclusive_scan_serial(InputIterator first, InputIterator last, OutputIterator result, T init, BinaryOperation binary_op)
{
    for (; first != last; ++first, ++result)
    {
        *result = init;
        init = binary_op(init, *first);
    }
    return result;
}

// Note: N4582 is missing the ", class T".  Issue was reported 2016-Apr-11 to cxxeditor@gmail.com
template <class InputIterator, class OutputIterator, class BinaryOperation, class T>
OutputIterator
inclusive_scan_serial(InputIterator first, InputIterator last, OutputIterator result, BinaryOperation binary_op, T init)
{
    for (; first != last; ++first, ++result)
    {
        init = binary_op(init, *first);
        *result = init;
    }
    return result;
}

template <class InputIterator, class OutputIterator, class BinaryOperation>
OutputIterator
inclusive_scan_serial(InputIterator first, InputIterator last, OutputIterator result, BinaryOperation binary_op)
{
    if (first != last)
    {
        auto tmp = *first;
        *result = tmp;
        return inclusive_scan_serial(++first, last, ++result, binary_op, tmp);
    }
    else
    {
        return result;
    }
}

template <class InputIterator, class OutputIterator>
OutputIterator
inclusive_scan_serial(InputIterator first, InputIterator last, OutputIterator result)
{
    typedef typename ::std::iterator_traits<InputIterator>::value_type input_type;
    return inclusive_scan_serial(first, last, result, ::std::plus<input_type>());
}

// Most of the framework required for testing inclusive and exclusive scan is identical,
// so the tests for both are in this file.  Which is being tested is controlled by the global
// flag inclusive, which is set to each alternative by main().
//static bool inclusive;

template <typename Type>
struct test_inclusive_scan_with_plus
{
    template <typename Policy, typename Iterator1, typename Iterator2, typename Iterator3, typename Size, typename T>
    void
    operator()(Policy&& exec, Iterator1 in_first, Iterator1 in_last, Iterator2 out_first, Iterator2 out_last,
               Iterator3 expected_first, Iterator3 /* expected_last */, Size n, T /* init */, T trash)
    {
        using namespace std;

        inclusive_scan_serial(in_first, in_last, expected_first);
        auto orr = inclusive_scan(exec, in_first, in_last, out_first);
        EXPECT_TRUE(out_last == orr, "inclusive_scan returned wrong iterator");
        EXPECT_EQ_N(expected_first, out_first, n, "wrong result from inclusive_scan");
        ::std::fill_n(out_first, n, trash);
    }
};

template <typename Type>
struct test_exclusive_scan_with_plus
{
    template <typename Policy, typename Iterator1, typename Iterator2, typename Iterator3, typename Size, typename T>
    void
    operator()(Policy&& exec, Iterator1 in_first, Iterator1 in_last, Iterator2 out_first, Iterator2 out_last,
               Iterator3 expected_first, Iterator3 /* expected_last */, Size n, T init, T trash)
    {
        using namespace std;

        exclusive_scan_serial(in_first, in_last, expected_first, init);
        auto orr = exclusive_scan(exec, in_first, in_last, out_first, init);
        EXPECT_TRUE(out_last == orr, "exclusive_scan returned wrong iterator");
        EXPECT_EQ_N(expected_first, out_first, n, "wrong result from exclusive_scan");
        ::std::fill_n(out_first, n, trash);
    }
};

template <typename T, typename Convert>
void
test_with_plus(T init, T trash, Convert convert)
{
    for (size_t n = 0; n <= 100000; n = n <= 16 ? n + 1 : size_t(3.1415 * n))
    {
        Sequence<T> in(n, convert);
        Sequence<T> expected(in);
        Sequence<T> out(n, [&](int32_t) { return trash; });

#ifdef _PSTL_TEST_INCLUSIVE_SCAN

        invoke_on_all_policies<0>()(test_inclusive_scan_with_plus<T>(), in.begin(), in.end(), out.begin(), out.end(),
                                    expected.begin(), expected.end(), in.size(), init, trash);
        invoke_on_all_policies<1>()(test_inclusive_scan_with_plus<T>(), in.cbegin(), in.cend(), out.begin(),
                                    out.end(), expected.begin(), expected.end(), in.size(), init, trash);
#endif

#ifdef _PSTL_TEST_EXCLUSIVE_SCAN

        invoke_on_all_policies<2>()(test_exclusive_scan_with_plus<T>(), in.begin(), in.end(), out.begin(),
                                    out.end(), expected.begin(), expected.end(), in.size(), init, trash);
        invoke_on_all_policies<3>()(test_exclusive_scan_with_plus<T>(), in.cbegin(), in.cend(), out.begin(), out.end(),
                                    expected.begin(), expected.end(), in.size(), init, trash);
#endif
    }

#if _ONEDPL_BACKEND_SYCL && !_ONEDPL_FPGA_DEVICE
    unsigned long n = 100000000;
    Sequence<T> in(n, convert);
    Sequence<T> expected(in);
    Sequence<T> out(n, [&](int32_t) { return trash; });
#ifdef _PSTL_TEST_INCLUSIVE_SCAN
    invoke_on_all_hetero_policies<4>()(test_inclusive_scan_with_plus<T>(), in.begin(), in.end(), out.begin(), out.end(),
                                expected.begin(), expected.end(), in.size(), init, trash);
#endif
#ifdef _PSTL_TEST_EXCLUSIVE_SCAN
    invoke_on_all_hetero_policies<5>()(test_exclusive_scan_with_plus<T>(), in.begin(), in.end(), out.begin(),
                                out.end(), expected.begin(), expected.end(), in.size(), init, trash);
#endif
#endif // _ONEDPL_BACKEND_SYCL && !_ONEDPL_FPGA_DEVICE
}

template <typename Type>
struct test_inclusive_scan_with_binary_op
{
    template <typename Policy, typename Iterator1, typename Iterator2, typename Iterator3, typename Size, typename T,
              typename BinaryOp>
    typename ::std::enable_if<!TestUtils::isReverse<Iterator1>::value, void>::type
    operator()(Policy&& exec, Iterator1 in_first, Iterator1 in_last, Iterator2 out_first, Iterator2 out_last,
               Iterator3 expected_first, Iterator3 /* expected_last */, Size n, T init, BinaryOp binary_op, T trash)
    {
        using namespace std;

        inclusive_scan_serial(in_first, in_last, expected_first, binary_op, init);
        auto orr = inclusive_scan(exec, in_first, in_last, out_first, binary_op, init);

        EXPECT_TRUE(out_last == orr, "inclusive_scan with binary operator returned wrong iterator");
        EXPECT_EQ_N(expected_first, out_first, n, "wrong result from inclusive_scan with binary operator");
        ::std::fill_n(out_first, n, trash);
    }

    template <typename Policy, typename Iterator1, typename Iterator2, typename Iterator3, typename Size, typename T,
              typename BinaryOp>
    typename ::std::enable_if<TestUtils::isReverse<Iterator1>::value, void>::type
    operator()(Policy&& /* exec */, Iterator1 /* in_first */, Iterator1 /* in_last */, Iterator2 /* out_first */, Iterator2 /* out_last */,
               Iterator3 /* expected_first */, Iterator3 /* expected_last */, Size /* n */, T /* init */, BinaryOp /* binary_op */, T /* trash */)
    {
    }
};

template <typename Type>
struct test_exclusive_scan_with_binary_op
{
    template <typename Policy, typename Iterator1, typename Iterator2, typename Iterator3, typename Size, typename T,
              typename BinaryOp>
    typename ::std::enable_if<!TestUtils::isReverse<Iterator1>::value, void>::type
    operator()(Policy&& exec, Iterator1 in_first, Iterator1 in_last, Iterator2 out_first, Iterator2 out_last,
               Iterator3 expected_first, Iterator3 /* expected_last */, Size n, T init, BinaryOp binary_op, T trash)
    {
        using namespace std;

        exclusive_scan_serial(in_first, in_last, expected_first, init, binary_op);

        auto orr = exclusive_scan(exec, in_first, in_last, out_first, init, binary_op);

        EXPECT_TRUE(out_last == orr, "exclusive_scan with binary operator returned wrong iterator");
        EXPECT_EQ_N(expected_first, out_first, n, "wrong result from exclusive_scan with binary operator");
        ::std::fill_n(out_first, n, trash);
    }

    template <typename Policy, typename Iterator1, typename Iterator2, typename Iterator3, typename Size, typename T,
              typename BinaryOp>
    typename ::std::enable_if<TestUtils::isReverse<Iterator1>::value, void>::type
    operator()(Policy&& /* exec */, Iterator1 /* in_first */, Iterator1 /* in_last */, Iterator2 /* out_first */, Iterator2 /* out_last */,
               Iterator3 /* expected_first */, Iterator3 /* expected_last */, Size /* n */, T /* init */, BinaryOp /* binary_op */, T /* trash */)
    {
    }
};

template <typename In, typename Out, typename BinaryOp>
void
test_matrix(Out init, BinaryOp binary_op, Out trash)
{
    for (size_t n = 0; n <= 100000; n = n <= 16 ? n + 1 : size_t(3.1415 * n))
    {
        Sequence<In> in(n, [](size_t k) { return In(k, k + 1); });

        Sequence<Out> out(n, [&](size_t) { return trash; });
        Sequence<Out> expected(n, [&](size_t) { return trash; });

#ifdef _PSTL_TEST_INCLUSIVE_SCAN
        invoke_on_all_policies<4>()(test_inclusive_scan_with_binary_op<In>(), in.begin(), in.end(), out.begin(),
                                    out.end(), expected.begin(), expected.end(), in.size(), init, binary_op, trash);
        invoke_on_all_policies<5>()(test_inclusive_scan_with_binary_op<In>(), in.cbegin(), in.cend(), out.begin(),
                                    out.end(), expected.begin(), expected.end(), in.size(), init, binary_op, trash);
#endif

#ifdef _PSTL_TEST_EXCLUSIVE_SCAN
        invoke_on_all_policies<6>()(test_exclusive_scan_with_binary_op<In>(), in.begin(), in.end(), out.begin(),
                                    out.end(), expected.begin(), expected.end(), in.size(), init, binary_op, trash);
        invoke_on_all_policies<7>()(test_exclusive_scan_with_binary_op<In>(), in.cbegin(), in.cend(), out.begin(),
                                    out.end(), expected.begin(), expected.end(), in.size(), init, binary_op, trash);
#endif
    }
}

int
main()
{
#if !_PSTL_ICC_19_TEST_SIMD_UDS_WINDOWS_RELEASE_BROKEN
#if !_ONEDPL_BACKEND_SYCL
    // Test with highly restricted type and associative but not commutative operation
    test_matrix<Matrix2x2<int32_t>, Matrix2x2<int32_t>>(Matrix2x2<int32_t>(), multiply_matrix<int32_t>,
                                                            Matrix2x2<int32_t>(-666, 666));
#endif
#endif

    // Since the implicit "+" forms of the scan delegate to the generic forms,
    // there's little point in using a highly restricted type, so just use double.
    test_with_plus<float64_t>(0.0, -666.0, [](uint32_t k) { return float64_t((k % 991 + 1) ^ (k % 997 + 2)); });
    test_with_plus<int32_t>(0.0, -666.0, [](uint32_t k) { return int32_t((k % 991 + 1) ^ (k % 997 + 2)); });

    ::std::cout << done() << ::std::endl;
    return 0;
}
