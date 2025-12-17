// WaitingRoom.js
import "./WaitingRoom.css";
import UserCard from "../../Lobby/UserCard"; 
import RoomTitle from "./RoomTitle";
import GameRulesPanel from "./GameRulesPanel"; 
import MemberListPanel from "./MemberListPanel"; 

export default function WaitingRoom({ role }) { 
  const roomInfo = {
    name: "The Price is Right (A)",
    code: "XYZ456",
    isHost: true,
  };

  return (
    <div 
      className="waiting-room" 
      style={{ 
        backgroundImage: "url('/bg/waitingroom.png')",
        backgroundSize: '100% 100%', // Ép ảnh fit vừa khít màn hình
        backgroundPosition: 'center',
        backgroundRepeat: 'no-repeat',
        backgroundAttachment: 'fixed'
      }}
    >
      {/* Phần Header phía trên cùng */}
      <UserCard /> 
      <RoomTitle title="Waiting Room" /> 
      
      {/* Nội dung chính chia làm 3 cột */}
      <div className="waiting-room-content"> 
        
        {/* CỘT 1: GAME RULES (BÊN TRÁI) */}
        <div className="wr-left">
          <GameRulesPanel isHost={roomInfo.isHost} />
        </div>

        {/* CỘT 2: MEMBER LIST (Ở GIỮA) */}
        <div className="wr-center">
          <MemberListPanel 
            isHost={roomInfo.isHost} 
            roomName={roomInfo.name} 
            roomCode={roomInfo.code} 
          />
        </div>

        {/* CỘT 3: CÁC NÚT HÀNH ĐỘNG (BÊN PHẢI) */}
        <div className="wr-right-actions">
          {roomInfo.isHost && (
            <button className="start-game-btn">START GAME</button>
          )}
          <button className="invite-btn">INVITE FRIENDS</button>
          <button className="leave-btn">LEAVE ROOM</button>
        </div>

      </div>
    </div>
  );
}