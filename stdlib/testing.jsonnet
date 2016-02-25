/*
Test:
    {
        name: string,required
        cases: object[],optional
        // if cases is not null
        evalute(case):: (object -> CaseResult)
        // if cases is null
        evalute():: (() -> CaseResult)
    }

Case:
    {
        name:  string,optional

        ...testdata...
    }

CaseResult:
    {
        name:
        passed: bool,required
        message: string,optional
    }

TestResult:
    {
        name: string,required
        passed: bool,required
        results: CaseResult[]
    }

*/
{

    local testing = $,

    format(results)::
        local testResultTemplate =
            "--- %(result)s '%(name)s'\n";
        local caseResultTemplate =
            "\t%(result)s\t'%(name)s'";
        local caseResultMessageTemplate =
            ":\n    %(message)s\n";

        local formatResults(str, cur, results, formatResult) =
            if std.length(results) == cur then
                str
            else
                formatResults(
                    str + formatResult(cur, results[cur]),
                    cur + 1,
                    results,
                    formatResult
                ) tailstrict;

        local formatCaseResult(cur, caseResult)  =
            local result =
                    if caseResult.passed then
                        "pass"
                    else
                        "fail";
            local name =
                    if std.objectHas(caseResult, "name") then
                        caseResult.name
                    else
                        "case %s" % cur;
            local caseResultString = caseResultTemplate % {result: result, name: name};

            local message =
                if std.objectHas(caseResult, "message") then
                    caseResultMessageTemplate % caseResult
                else
                    "\n";

            caseResultString + message;

        local formatTestCases(str, results) =
            formatResults(str, 0, results, formatCaseResult);

        local formatTestResult(cur, testResult) =
            local str = testResultTemplate % {
                result:
                    if testResult.passed then
                        "PASS"
                    else
                        "FAIL",
                name: testResult.name,
            };
            formatTestCases(str, testResult.results);

        local formatTestResults(str, cur, results) =
            formatResults(str, cur, results, formatTestResult);

        formatTestResults("", 0, results),

    passed(results)::
        std.foldl(function(cur,result) cur && result.passed, results, true),

    evaluateToCaseResult(obj)::
        if obj == null then
            { passed: true }
        else if std.type(obj) == "string" then
            { passed: false, message: obj }
        else
            error("evaluate must return a failure message or {}."),

    evaluateTest(name, test)::
        local caseResults =
            if std.type(test) == "object" then
                [
                    testing.evaluateToCaseResult(test.evaluate(test.cases[i])) + test.cases[i]
                    for i in std.range(0, std.length(test.cases) - 1)
                ]
            else if std.type(test) == "function" then
                [ testing.evaluateToCaseResult(test()) ]
            else
                error "tests muts be objects or functions";
        {
            name: name,
            passed: testing.passed(caseResults),
            results: caseResults
        },

    evaluate(tests)::
        if std.type(tests) == "object" then
            local results =
                [ testing.evaluateTest(name, tests[name]) for name in std.objectFields(tests) ];
            results
        else
            error("input must be a test object"),

    run(input)::
        local results = testing.evaluate(input);
        if testing.passed(results) then
            testing.format(results)
        else
            error("\n" + testing.format(results)),
}
