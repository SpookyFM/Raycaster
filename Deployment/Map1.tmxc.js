(function(name,data){
 if(typeof onTileMapLoaded === 'undefined') {
  if(typeof TileMaps === 'undefined') TileMaps = {};
  TileMaps[name] = data;
 } else {
  onTileMapLoaded(name,data);
 }
 if(typeof module === 'object' && module && module.exports) {
  module.exports = data;
 }})("Map1",
{ "height":10,
 "layers":[
        {
         "data":[1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 33, 0, 0, 0, 1, 1, 0, 0, 0, 0, 33, 0, 0, 0, 1, 1, 0, 0, 0, 0, 33, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 33, 0, 0, 0, 1, 1, 0, 0, 0, 0, 33, 0, 0, 0, 1, 1, 0, 0, 0, 0, 33, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
         "height":10,
         "name":"Kachelebene 1",
         "opacity":1,
         "type":"tilelayer",
         "visible":true,
         "width":10,
         "x":0,
         "y":0
        }],
 "nextobjectid":1,
 "orientation":"orthogonal",
 "renderorder":"right-down",
 "tiledversion":"1.0.3",
 "tileheight":64,
 "tilesets":[
        {
         "firstgid":1,
         "source":"..\/Tiled\/Walls.tsx"
        }],
 "tilewidth":64,
 "type":"map",
 "version":1,
 "width":10
});