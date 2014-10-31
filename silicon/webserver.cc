

int main()
{

  server s(argc, argv);

  s["/"] = "hello world";

  s["/image"] = send_file("/tmp/xxx,jpg");
  s["/dir"] = directory_index("dir");

  s["/user_infos"] = [] (session sess,
                         iod_type(name = string()) params)
  {
    return sess.id;
  };

  s["/bindings.js"] = generate_javascript_bindings(s);

  // s.preambule();
  // s.epilogue();

  s.serve();
}
