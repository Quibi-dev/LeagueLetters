<?php

define("API_KEY", "");                                                          //fill in your API key here
define("API_URL", "https://euw1.api.riotgames.com/lol/");                       //base URL, changes per region

$playerName = "";       //Summoner name

$result = array();

$summoner = getSummoner($playerName);                           //Get summoner to figure out what the account ID is.

if(!isset($summoner["accountId"])){
    header('Content-type:application/json;charset=utf-8');      //Could not retrieve account ID, return server response.
    echo json_encode($summoner);
    die;
}

$matchHistory = getMatchHistory($summoner,5);       //Retrieve latest 5 games of summoner

if(getActiveGame($summoner)){                       //Checks if summoner is currently in active game
    array_push($result, true);
}else{
    array_push($result, false);
}

if(isset($matchHistory["matches"])){
    foreach($matchHistory["matches"] as $key=>$match){
        $playerId = NULL;
        $matchDetails = getMatchDetails($match["gameId"]);
       
        if($matchDetails["gameDuration"] < 400){
            array_push($result, "remake");            //I could not find the remake parameter in Rito's API, so if game ends below 400 seconds, I'll push a remake.
            continue;
        }
       
        if(isset($matchDetails["participantIdentities"])){
            $players = $matchDetails["participantIdentities"];              
            foreach($players as $player){                                           //Retrieve participant ID
                if($player["player"]["summonerName"] == $playerName){
                    $playerId = $player["participantId"];
                }
            }
            foreach($matchDetails["participants"] as $player){                      //Retrieve Team ID
                if($playerId == $player["participantId"]){
                    $teamId = $player["teamId"];
                }
            }
            foreach($matchDetails["teams"] as $team){                               //Check if retrieved team is the team that won the game.
                if($team["win"] != "Win"){
                    continue;
                }
                if($team["teamId"] == $teamId){
                    array_push($result, "win");
                }else{
                    array_push($result, "loss");
                }
            }
        }else{
            die;
        }
    }
}

header('Content-type:application/json;charset=utf-8');
echo json_encode($result);
die;


function getSummoner($name){
    $summonerIDResponse = callAPI('GET', API_URL.'summoner/v4/summoners/by-name/'.rawurlencode($name).'', false);
    return(json_decode($summonerIDResponse, true));
}

function getMatchHistory($summoner, $numberOfGames){
    $matchHistoryResponse = callAPI('GET', API_URL.'match/v4/matchlists/by-account/'.$summoner["accountId"].'?endIndex='.$numberOfGames.'', false);
    return(json_decode($matchHistoryResponse, true));
}

function getMatchDetails($matchId){
    $matchDetailsResponse = callAPI('GET', API_URL.'match/v4/matches/'.$matchId.'', false);
    return(json_decode($matchDetailsResponse, true));
}

function getActiveGame($summoner){
    $liveGameResponse   = callAPI('GET', API_URL.'spectator/v4/active-games/by-summoner/'.$summoner["id"].'', false);
    $liveGame           = json_decode($liveGameResponse, true);
    if(isset($liveGame["gameId"])){
        return true;
    }else{
        return false;
    }
}

function callAPI($method, $url, $data){
   $curl = curl_init();
   //global $API_KEY;
   switch ($method){
      case "POST":
         curl_setopt($curl, CURLOPT_POST, 1);
         if ($data)
            curl_setopt($curl, CURLOPT_POSTFIELDS, $data);
         break;
      case "PUT":
         curl_setopt($curl, CURLOPT_CUSTOMREQUEST, "PUT");
         if ($data)
            curl_setopt($curl, CURLOPT_POSTFIELDS, $data);			 					
         break;
      default:
         if ($data)
            $url = sprintf("%s?%s", $url, http_build_query($data));
   }

   // OPTIONS:
   curl_setopt($curl, CURLOPT_URL, $url);
   curl_setopt($curl, CURLOPT_HTTPHEADER, array(
        "Origin: https://developer.riotgames.com",
        "Accept-Charset: application/x-www-form-urlencoded; charset=UTF-8",
        "X-Riot-Token: ".API_KEY."",
        "Accept-Language: nl-NL,nl;q=0.9,en-US;q=0.8,en;q=0.7"
   ));
   curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
   curl_setopt($curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);

   // EXECUTE:
   $result = curl_exec($curl);
   if(!$result){die("Connection Failure");}
   curl_close($curl);
   return $result;
}
?>