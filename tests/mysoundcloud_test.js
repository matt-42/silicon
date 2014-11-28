
msc.set_server("http://127.0.0.1:8888/");
msc.set_async(true); // To simplify testing.

var user_info = {email: "abc@gmail.com", password: "1234"};
var user_info2 = {email: "def@gmail.com", password: "1234"};

if (fs.existsSync("./cookie.db"))
    CookieJar.load(fs.readFileSync("./cookie.db", { encoding: "utf8" }));

function expect_(promise)
{
  this.promise = promise;
}

expect_.prototype.to_fail = async function () {
  var _this = this;
  await this.promise.then(() => { throw Error("this should fail.") },
                          (() => { _this.promise = Promise.resolve(true); })).catch(true);
  return this;
}

expect_.prototype.to_succeed = async function ()
{
  await this.promise.then((() => (true)) ,
                           () => { throw Error("this should succeed.") });
  return this;
}

expect_.prototype.then = async function (a, b)
{
  return this.promise.then(a, b);
}

expect_.prototype.to_match = function (m)
{
  (this.promise.then((x) => { if (x != m) throw Error("should match.")},
                     (() => (true))));
  return this;
}
expect_.prototype.to_match_keys = async function (m)
{
  await (this.promise.then((x) => true, () => (true)));
  return this;  
} // Fixme


function expect(promise)
{
  return new expect_(promise);
}

// Cleanup last tests.
async function cleanup()
{
  console.log("cleaning up..");
  try
  {
    await msc.login(user_info);
    await msc.signout({password: user_info.password});
    await msc.logout();
    
  } catch (e) {}

  try
  {
    await msc.login(user_info2);
    await msc.signout({password: user_info2.password});
  } catch (e) {}
}

async function test_expect()
{
  try
  {
    await expect(Promise.reject(false)).to_fail();
    await expect(Promise.resolve(true)).to_succeed();
    console.log("test_expect succeed.");    
  }
  catch (e) {
    console.log("test_expect failed.");
    console.log(e);
  }
}

async function test()
{
  try
  {
    console.log("testing..");
    // Login without account should fail

    var song_id = 0;
    var song_info = {title: "mysong", filename: "mysong.mp3"};
    var song_content = "dsfdls0987f87g6sd9fg69a76f0r67vr0a76g0esrg87"
    var song_info2 = {id: song_id, title: "mysong_updated", filename: "mysong_updated.mp3"};

    console.log("testing 2..");
    
    await expect(msc.login(user_info)).to_fail();

    console.log("testing 3..");

    console.log("Signup should work.");
    await expect(msc.signup(user_info)).to_succeed();
    await expect(msc.signup(user_info2)).to_succeed();

    console.log("Signup twice with the same email should fail.");
    await expect(msc.signup(user_info)).to_fail();

    console.log("Login.");
    await expect(msc.login(user_info)).to_succeed();

    console.log("Create a song.");
    song_id = (await msc.song_create(song_info)).id;
    if (!song_id) throw Error("missing id after create");

    console.log("Get song by id.");
    var s = await msc.song_get_by_id({id: song_id});
    if (s.title != song_info.title && s.filename != song_info.filename)
      throw Error("song_info does not match");

    console.log("Update.");
    song_info2.id = song_id;
    await expect(msc.song_update(song_info2)).to_succeed();

    s = await msc.song_get_by_id({id: song_id});
    if (s.title != song_info2.title && s.filename != song_info2.filename)
      throw Error("song_info2 does not match");

    console.log("Update without being logged should fail.");
    await msc.logout();
    await expect(msc.song_update(song_info2)).to_fail();

    console.log("Update without write permission should fail.");
    await msc.login(user_info2);
    await expect(msc.song_update(song_info2)).to_fail();

    await msc.logout();
    await msc.login(user_info);
    
    console.log("Upload.");
    var upload_params = JSON.stringify({id: song_id}) + song_content;
    await expect(msc.upload(upload_params)).to_succeed();

    console.log("Upload without being logged should fail.");
    await msc.logout();
    await expect(msc.upload(upload_params)).to_fail();

    console.log("Upload without write permission should fail.");
    await msc.login(user_info2);
    await expect(msc.upload(upload_params)).to_fail();
    
    await msc.logout();

    console.log("Stream");
    // var streamed = await msc.stream({id: song_id});
    // if (streamed != song_content) throw Error("song content does not match.");
    
    console.log("Destroy song.");
    await msc.login(user_info);
    await msc.song_delete({id: song_id});
    await expect(msc.song_get_by_id({id: song_id})).to_fail();
    // await expect(msc.stream({id: song_id})).to_fail();
    

    console.log("All tests passed.");
  }
  catch (e) { console.error("Test failed: "); console.log(e); throw e; }
}

(async () =>
 {
  await test_expect();
  await cleanup();
  await test();
})();
