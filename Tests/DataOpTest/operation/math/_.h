#pragma once

namespace Test::Operation::Math
{
    void test_abs();
    void test_acos();   void test_acos_grad();
    void test_add();
    void test_asin();   void test_asin_grad();
    void test_divide();
    void test_multiply();
    void test_negative();
    void test_reduce_sum();
    void test_sigmoid();    void test_sigmoid_grad();
    void test_sign();
    void test_substract();
    void test_tanh();       void test_tanh_grad();

    void test()
    {
        test_abs();
        test_acos();    test_acos_grad();
        test_add();
        test_asin();    test_asin_grad();
        test_divide();
        test_multiply();
        test_negative();
        test_reduce_sum();
        test_sigmoid();     test_sigmoid_grad();
        test_sign();
        test_substract();
        test_tanh();        test_tanh_grad();
    }
}