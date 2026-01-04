// Lobby.js (FIXED: UserCard nằm ngoài cột, đặt trực tiếp dưới .lobby)
import "./Lobby.css";
import UserCard from "./UserCard";
import AppTitle from "./AppTitle";
import RoomPanel from "./RoomPanel";
import { viewHistory } from "../../services/historyService";
import { useNavigate } from "react-router-dom";

export default function Lobby() {
  const navigate = useNavigate(); 
  return (
    <div className="lobby">
      {/* 1. USER CARD (Định vị tuyệt đối ở góc trên bên trái) */}
      <UserCard /> 
      
      {/* 2. APP TITLE (Nằm trên cùng, căn giữa) */}
      <AppTitle /> 
      
      {/* 3. LOBBY CONTENT (Khối 2 cột, căn giữa) */}
      <div className="lobby-content"> 
        <div className="lobby-left">
          {/* UserCard đã bị loại bỏ khỏi đây */}
          
          <div className="side-actions">
            {/* <button onClick={viewHistory}>VIEW HISTORY</button> */}
            <button onClick={() => navigate("/history")}>
              VIEW HISTORY
            </button>

            <button>VIEW TUTORIAL</button>

            <button>SETTING</button>
            <button className="logout">LOG OUT</button>
          </div>
        </div>

        <div className="lobby-right">
          {/* RoomPanel nằm ở cột phải */}
          <RoomPanel />
        </div>
      </div>
    </div>
  );
}