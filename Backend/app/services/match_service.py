# app/services/match_service.py
from app.models import db
from app.models.room import Room, RoomMember
from app.models.match import Match, MatchPlayer
from app.models.question import Question, MatchQuestion
from datetime import datetime
import random


class MatchService:
    
    @staticmethod
    def start_game(room_id, account_id):
        """
        Host starts the game
        Phase 1: Create match, validate conditions (NO questions returned)
        """
        try:
            room = Room.query.get(room_id)
            if not room:
                return {'success': False, 'error': 'Room not found'}
            
            # Check host permission
            if room.host_id != account_id:
                return {'success': False, 'error': 'Only host can start game'}
            
            # Check room status
            if room.status != 'waiting':
                return {'success': False, 'error': 'Room not in waiting state'}
            
            # Get members
            members = RoomMember.query.filter_by(room_id=room_id).all()
            player_count = len(members)
            
            # Check player count based on mode
            mode = room.mode or 'scoring'
            if mode == 'elimination' and player_count != 4:
                return {'success': False, 'error': 'Elimination mode requires exactly 4 players'}
            if mode == 'scoring' and not (4 <= player_count <= 6):
                return {'success': False, 'error': 'Scoring mode requires 4-6 players'}
            
            # Check all members ready (except host)
            for member in members:
                if member.account_id != account_id and not member.ready:
                    return {'success': False, 'error': 'Not all players are ready'}
            
            # Create Match
            match = Match(
                room_id=room_id,
                mode=mode,
                max_players=player_count,
                advanced=room.advanced or {},
                round_time=room.round_time or 15,
                started_at=datetime.utcnow()
            )
            db.session.add(match)
            db.session.flush()
            
            # Create MatchPlayers
            for member in members:
                player = MatchPlayer(
                    match_id=match.id,
                    account_id=member.account_id,
                    score=0,
                    eliminated=False,
                    forfeited=False,
                    winner=False
                )
                db.session.add(player)
            
            # Update room status
            room.status = 'in_game'
            room.updated_at = datetime.utcnow()
            
            db.session.commit()
            
            # Return ONLY match_id (NO questions)
            # Questions will be fetched separately via get_round_questions
            return {
                'success': True,
                'match_id': match.id,
                'message': 'Game started, waiting for countdown'
            }
            
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}
    
    @staticmethod
    def get_round_questions(match_id, round_no):
        """
        Get questions for a specific round (called by C Server after countdown)
        """
        try:
            match = Match.query.get(match_id)
            if not match:
                return {'success': False, 'error': 'Match not found'}
            
            # Check if questions already generated for this round
            existing = MatchQuestion.query.filter_by(
                match_id=match_id,
                round_no=round_no
            ).first()
            
            if existing:
                # Return existing questions
                questions = MatchQuestion.query.filter_by(
                    match_id=match_id,
                    round_no=round_no
                ).order_by(MatchQuestion.question_idx).all()
                
                return {
                    'success': True,
                    'questions': [q.to_dict() for q in questions]
                }
            
            # Generate new questions based on round type
            round_type, question_count = MatchService._get_round_config(round_no)
            
            questions = MatchService._generate_round_questions(
                match_id, round_no, round_type, question_count
            )
            
            db.session.commit()
            
            return {
                'success': True,
                'questions': [q.to_dict() for q in questions]
            }
            
        except Exception as e:
            db.session.rollback()
            return {'success': False, 'error': str(e)}
    
    @staticmethod
    def _get_round_config(round_no):
        """Map round number to type and question count"""
        configs = {
            1: ('MCQ', 10),      # Round 1: Speed Challenge
            2: ('BID', 5),       # Round 2: The Bid
            3: ('WHEEL', 1),     # Round 3: Bonus Wheel
            4: ('BONUS', 1),     # Round 4: Bonus Duel (if tie)
        }
        return configs.get(round_no, ('MCQ', 10))
    
    @staticmethod
    def _generate_round_questions(match_id, round_no, round_type, count):
        """Generate and save questions for a round"""
        questions = []
        
        # Get random questions from database
        available = Question.query.filter_by(
            type=round_type,
            active=True
        ).all()
        
        if len(available) < count:
            # If not enough questions, use all available
            selected = available
        else:
            selected = random.sample(available, count)
        
        for idx, q in enumerate(selected):
            mq = MatchQuestion(
                match_id=match_id,
                round_no=round_no,
                round_type=round_type,
                question_idx=idx + 1,
                question=q.data  # Store snapshot
            )
            db.session.add(mq)
            questions.append(mq)
        
        return questions
