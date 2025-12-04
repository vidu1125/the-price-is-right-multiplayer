from app.models import db, Account, Profile
import bcrypt

class AuthService:
    @staticmethod
    def register_user(email, password, name):
        # 1. Validation: Kiểm tra Email đã tồn tại chưa
        if Account.query.filter_by(email=email).first():
            return {"error": "Email này đã được sử dụng", "code": 400}

        # 2. Validation: Kiểm tra Tên hiển thị (Profile name) trùng không (Tùy chọn)
        # Nếu game yêu cầu tên nhân vật là duy nhất, hãy bỏ comment dòng dưới
        # if Profile.query.filter_by(name=name).first():
        #     return {"error": "Tên hiển thị đã có người dùng", "code": 400}

        # 3. Mã hóa mật khẩu
        hashed_pw = bcrypt.hashpw(password.encode('utf-8'), bcrypt.gensalt()).decode('utf-8')

        try:
            # --- BẮT ĐẦU TRANSACTION ---
            
            # 4. Tạo Account (Provider mặc định là 'local')
            new_account = Account(
                email=email,
                password=hashed_pw,
                provider='local',
                status='active'
            )
            db.session.add(new_account)
            
            # flush() đẩy dữ liệu xuống DB để lấy new_account.id nhưng chưa Commit
            db.session.flush() 

            # 5. Tạo Profile liên kết với Account vừa tạo
            new_profile = Profile(
                account_id=new_account.id,
                name=name,  # Tên hiển thị trong game
                badges=[],   # Mặc định danh sách rỗng như Model bạn khai báo
                points=0,
                matches=0,
                wins=0
            )
            db.session.add(new_profile)

            # 6. Commit: Lưu vĩnh viễn cả 2 bảng
            db.session.commit()

            return {
                "success": True,
                "message": "Đăng ký thành công",
                "user": new_account.to_dict(),
                "profile": new_profile.to_dict(),
                "code": 201
            }

        except Exception as e:
            db.session.rollback() # Hoàn tác nếu có lỗi, không để dữ liệu rác
            print(f"Error Registering: {str(e)}")
            return {"error": "Lỗi hệ thống, vui lòng thử lại sau", "code": 500}

    @staticmethod
    def login_user(email, password):
        # 1. Tìm tài khoản theo email
        account = Account.query.filter_by(email=email).first()
        
        if not account:
            return {"error": "Email hoặc mật khẩu không đúng", "code": 401}
        
        # 2. Kiểm tra mật khẩu
        if not bcrypt.checkpw(password.encode('utf-8'), account.password.encode('utf-8')):
            return {"error": "Email hoặc mật khẩu không đúng", "code": 401}
        
        # 3. Kiểm tra trạng thái tài khoản
        if account.status != 'active':
            return {"error": "Tài khoản của bạn không hoạt động", "code": 403}
        
        try:
            # 4. Lấy Profile
            profile = Profile.query.filter_by(account_id=account.id).first()
            
            return {
                "success": True,
                "message": "Đăng nhập thành công",
                "user": account.to_dict(),
                "profile": profile.to_dict() if profile else None,
                "code": 200
            }
        except Exception as e:
            print(f"Error Login: {str(e)}")
            return {"error": "Lỗi hệ thống, vui lòng thử lại sau", "code": 500}