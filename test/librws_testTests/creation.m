//
//  creation.m
//  librws_test
//
//  Created by Resident evil on 27/01/16.
//  Copyright © 2016 none. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "librws.h"

@interface creation : XCTestCase

@end

@implementation creation

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void) testCreate
{
    // This is an example of a functional test case.
    // Use XCTAssert and related functions to verify your tests produce the correct results.
	rws_socket * socket = rws_socket_create();
	XCTAssert(socket != NULL, @"Not created");
	rws_socket_disconnect_and_release(socket);

	const rws_bool expressionTrue = rws_true;
	XCTAssertTrue(expressionTrue, @"rws_true is not true");
	XCTAssertTrue(rws_true, @"rws_true is not true");

	const rws_bool expressionFalse = rws_false;
	XCTAssertFalse(expressionFalse, @"rws_false is not false");
	XCTAssertFalse(rws_false, @"rws_false is not false");
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
