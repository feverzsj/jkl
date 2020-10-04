#pragma once


namespace jkdf::json{

    // request json:
    // [method, {p1: v1, p2: v2, ...}] or
    // [method, p1, p2, ...]
    // params are optional

    // response json:
    // for failed : {error : anything_not_null_undefined}
    // for succeed: {result: data }
    // 

    // when it's over http, the response code should be:
    // https://www.jsonrpc.org/historical/json-rpc-over-http.html#response-codes
 

    // params: jarr(...) or jobj(...)
    template<class S0, class S1, class P, class I>
    void append_json_rpc_request(S0& b, S1 const& method, P const& params, I const& id)
    {
        make_jobj_writer(b).pas(
            "jsonrpc", "2.0",
            "method" , method,
            "params" , params,
            "id"     , id);
    }

    // notification is request that doesn't require response
    template<class S0, class S1, class P>
    void append_json_rpc_notify(S0& b, S1 const& method, P const& params)
    {
        make_jobj_writer(b).pas(
            "jsonrpc", "2.0",
            "method", method,
            "params", params);
    }

    template<class S0, class R, class I>
    void append_json_rpc_response_succeed(S0& b, R const& result, I const& id)
    {
        make_jobj_writer(b).pas(
            "jsonrpc", "2.0",
            "result" , result,
            "id"     , id);
    }

    template<class S0, class R, class I>
    void append_json_rpc_response_failed(S0& b, R const& result, I const& id)
    {
    }


} // namespace jkdf::json